/*
 * Channel
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h> // calloc

#ifndef POLY_H
#include "POLY.h"
#endif
#include "scalar.h"
#include "lock.h"
#include "condition.h"
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	int _Atomic flags;
	int capacity;
	int occupation;
	int front;
	union {
		Scalar* buffer; // for capacity > 1
		Scalar  value;  // for capacity == 1
	};
	Lock entry;
	union {
		// buffered channel
		struct {
			Condition non_empty;
			Condition non_full;
		};
		// blocking channel
		Event rendezvous[2];
	};
} Channel;

static inline void channel_close(Channel* this);
static inline void channel_destroy(Channel* this);
static inline bool channel_drained(Channel* this);
static inline int  channel_init(Channel* this, unsigned capacity);
static inline int  channel_receive(Channel* this, Scalar* x);
static inline int  channel_send_scalar(Channel* this, Scalar x);

// Accept any scalar type
#define channel_send(CHANNEL,EXPRESSION) channel_send_scalar((CHANNEL), coerce(EXPRESSION))

// handy macro
#define spawn_filter(I,O,T,...)\
	task_spawn(T,&(struct T){.input=I,.output=O __VA_OPT__(,)__VA_ARGS__ })

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

// Constants for flags
enum channel_flag {
	CHANNEL_BUFFERED  = (1<<0),
	CHANNEL_BLOCKING  = (1<<1),
	CHANNEL_CLOSED    = (1<<2),
	CHANNEL_DRAINED   = (1<<3),
};

#ifdef DEBUG
#	define ASSERT_CHANNEL_INVARIANT\
		assert(0 <= this->occupation && this->occupation <= this->capacity);\
		if (this->capacity > 1) {\
			assert(0 <= this->front && this->front <  this->capacity);\
			assert(this->buffer != (Scalar*)0);\
		}\
		assert(!(_channel_empty(this) && _channel_full(this)));
#else
#	define ASSERT_CHANNEL_INVARIANT
#endif

// Same error management strategy in all this module
#define catch(X) if ((err=(X))!=STATUS_SUCCESS) goto onerror

//
// Predicates
//
static ALWAYS inline bool _channel_empty(Channel* this)
{
	return this->occupation == 0;
}

static ALWAYS inline bool _channel_full(Channel* this)
{
	return this->occupation == this->capacity;
}

//
// Channel life
//

static inline int channel_init(Channel* this, unsigned capacity)
{
	void d_lock(void)   { lock_destroy(&this->entry); }
	void d_empty(void)  { condition_destroy(&this->non_empty); }
	void d_buffer(void) { free(this->buffer); }

	// cleanup thunks to call before return
	void(*thunks[4])(void) = { 0 };
	int thunk_index = 0;
#	define at_cleanup(F) thunks[thunk_index++]=F

	int err;

	this->occupation = this->flags = 0;
	this->capacity = capacity;
	catch (lock_init(&this->entry));
	at_cleanup(d_lock);

	switch (this->capacity) {
		case 0:
			catch (event_init2(this->rendezvous, &this->entry));
			this->capacity = 1;
			this->flags |= CHANNEL_BLOCKING;
			break;
		default: // > 1
			this->front = 0;
			this->buffer = calloc(this->capacity, sizeof(Scalar));
			if (this->buffer == (Scalar*)0) {
				err = STATUS_NOMEM;
				goto onerror;
			}
			at_cleanup(d_buffer);
			fallthrough;
		case 1:
			catch (condition_init(&this->non_empty));
			at_cleanup(d_empty);
			catch (condition_init(&this->non_full));
			this->flags |= CHANNEL_BUFFERED;
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	return STATUS_SUCCESS;
onerror:
#	undef at_cleanup
	if (thunk_index > 0) {
		assert(thunk_index < sizeof(thunks)/sizeof(thunks[0]));
		int i;
		for (i=0; thunks[i] != (void(*)(void))0; ++i) ;
		for (--i; i >= 0; --i) (*thunks[i])();
	}
	return err;
}

static inline void channel_destroy(Channel* this)
{
	assert(_channel_empty(this));

	if (this->flags & CHANNEL_BUFFERED) {
		condition_destroy(&this->non_full);
		condition_destroy(&this->non_empty);
	} else if (this->flags & CHANNEL_BLOCKING) {
		event_destroy(&this->rendezvous[1]);
		event_destroy(&this->rendezvous[0]);
	}
	lock_destroy(&this->entry);
	if (this->capacity > 1) {
		free(this->buffer);
		this->buffer = (Scalar*)0;
	}
}

static inline void channel_close(Channel* this)
{
	this->flags |= CHANNEL_CLOSED;
	//?lock_acquire(&this->entry);
	if (this->occupation == 0) {
		this->flags |= CHANNEL_DRAINED;
	}
	//?lock_release(&this->entry);
}

static ALWAYS inline bool channel_drained(Channel* this)
{
	return (this->flags & CHANNEL_DRAINED);
}

//
// Monitor helpers
//
#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	if ((err=lock_acquire(&this->entry))!=STATUS_SUCCESS) {\
		return err;\
	}\
	if (this->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else while (PREDICATE(this)) {\
		if ((err=condition_wait(&CONDITION, &this->entry))!=STATUS_SUCCESS) {\
			lock_destroy(&this->entry);\
			return err;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if (this->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else if ((err=condition_notify(&CONDITION))!=STATUS_SUCCESS) {\
		lock_release(&this->entry);\
		return err;\
	}\
	if ((err=lock_release(&this->entry))!=STATUS_SUCCESS) {\
		return err;\
	}

static inline int channel_send_scalar(Channel* this, Scalar x)
{
	if (this->flags & CHANNEL_CLOSED) {
		panic("channel_send cannot use a closed channel");
	}

	int err;
	ENTER_CHANNEL_MONITOR (_channel_full, this->non_full)

	if (this->flags & CHANNEL_BLOCKING) {
		assert(this->capacity == 1);
		// protocol
		//    thread a: wait(0)-A-notify(1)
		//    thread b: notify(0)-wait(1)-B
		catch (event_wait(&this->rendezvous[0]));
		this->value = x;
		catch (event_notify(&this->rendezvous[1]));
	} else if (this->capacity == 1) {
		assert(_channel_empty(this));
		this->value = x;
	} else {
		assert(this->capacity > 1);
		this->buffer[this->front] = x;
		this->front = (this->front+1) % this->capacity;
	}
	++this->occupation;
	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (this->non_empty)

	return STATUS_SUCCESS;
onerror:
	lock_release(&this->entry);
	return err;
}

static inline int channel_receive(Channel* this, Scalar* x)
{
	if (this->flags & CHANNEL_DRAINED) {
		if (x) x->word = 0x0;
		return STATUS_SUCCESS;
	}

	int err;
	ENTER_CHANNEL_MONITOR (_channel_empty, this->non_empty)

	if (this->flags & CHANNEL_BLOCKING) {
		assert(this->capacity == 1);
		// protocol
		//    thread a: wait(0)-A-notify(1)
		//    thread b: notify(0)-wait(1)-B
		catch (event_notify(&this->rendezvous[0]));
		catch (event_wait(&this->rendezvous[1]));
		if (x) *x = this->value;
	} else if (this->capacity == 1) {
		assert(_channel_full(this));
		if (x) *x = this->value;
	} else if (x) {
		assert(this->capacity > 1);
		register int back = this->front - this->occupation;
		back = (back >= 0) ? back : back+this->capacity;
		*x = this->buffer[back];
	} // else ignore
	--this->occupation;
	ASSERT_CHANNEL_INVARIANT

	if ((this->flags & CHANNEL_CLOSED) && this->occupation == 0) {
		this->flags |= CHANNEL_DRAINED;
	}

	LEAVE_CHANNEL_MONITOR (this->non_full)

	return STATUS_SUCCESS;
onerror:
	lock_release(&this->entry);
	return err;
}

////////////////////////////////////////////////////////////////////////
// Dynamic allocation
////////////////////////////////////////////////////////////////////////

static inline Channel* channel_alloc(int capacity)
{
	Channel* channel = calloc(capacity ? capacity : 1, sizeof(Channel));
	if (channel == (Channel*)0) {
		panic("channel_alloc out of memory");
	}
	int err = channel_init(channel, capacity);
	if (err != STATUS_SUCCESS) {
		free(channel);
		panic("channel_alloc cannot initialize channel");
	}
	return channel;
}

static inline void channel_free(Channel* channel)
{
	if (channel) {
		channel_destroy(channel);
		free(channel);
	}
}

#undef catch
#undef ASSERT_CHANNEL_INVARIANT
#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
