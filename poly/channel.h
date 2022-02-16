#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h> // calloc

#ifndef POLY_H
#include "POLY.h"
#endif
#include "scalar.h"
#include "monitor/lock.h"
#include "monitor/condition.h"
#include "monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	atomic(unsigned) flags;
	unsigned capacity;
	unsigned occupation;
	unsigned front;
	union {
		Scalar* buffer; // for capacity > 1
		Scalar  value;  // for capacity == 1
	};
	Lock entry; // RecursiveLock?
	union {
		// buffered channel
		struct {
			Condition non_empty;
			Condition non_full;
		};
		// blocking channel
		Notice rendezvous[2];
	};
} Channel;

static void channel_close(Channel* this);
static void channel_destroy(Channel* this);
static bool channel_drained(Channel* this);
static int  channel_init(Channel* this, unsigned capacity);
static int  channel_receive(Channel* this, Scalar* message);
static int  channel_send(Channel* this, Scalar message);

// handy macro
#define spawn_filter(I,O,T,...)\
	thread_spawn(T, &(struct T){.input=I,.output=O __VA_OPT__(,)__VA_ARGS__ })

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
#define catch(X)\
	if ((err=(X)) != STATUS_SUCCESS)\
		goto onerror

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

static int
channel_init (Channel* this, unsigned capacity)
{
	void destroy_lock(void)   { lock_destroy(&this->entry); }
	void destroy_empty(void)  { condition_destroy(&this->non_empty); }
	void destroy_buffer(void) { free(this->buffer); }
	void destroy_notice(void) { notice_destroy(this->rendezvous+0); }

	// cleanup thunks to call before return
	void   (*_thunk_stack[4])(void) = { 0 };
	unsigned _thunk_index = 0;

	inline void at_cleanup(void thunk(void)) {
		_thunk_stack[_thunk_index++] = thunk;
	}
	void cleanup(void) {
		if (_thunk_index > 0) {
			assert(_thunk_index < sizeof(_thunk_stack)/sizeof(_thunk_stack[0]));
			for (signed i=_thunk_index-1; i >= 0; --i) {
				_thunk_stack[i]();
			}
		}
	}
	//
	int err;

	this->occupation = this->flags = 0;
	this->capacity = capacity;
	catch (lock_init(&this->entry));
	at_cleanup(destroy_lock);

	switch (this->capacity) {
		case 0:
			catch (notice_init(this->rendezvous+0, &this->entry));
			at_cleanup(destroy_notice);
			catch (notice_init(this->rendezvous+1, &this->entry));
			this->capacity = 1;
			this->flags |= CHANNEL_BLOCKING;
			break;
		default: // > 1
			this->front = 0;
			this->buffer = calloc(this->capacity, sizeof(Scalar));
			if (!this->buffer) catch (STATUS_NOMEM);
			at_cleanup(destroy_buffer);
			fallthrough;
		case 1:
			catch (condition_init(&this->non_empty));
			at_cleanup(destroy_empty);
			catch (condition_init(&this->non_full));
			this->flags |= CHANNEL_BUFFERED;
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	return STATUS_SUCCESS;

onerror:
	cleanup();
	return err;
}

static inline void
channel_destroy (Channel* this)
{
	assert(_channel_empty(this));

	if (this->flags & CHANNEL_BUFFERED) {
		condition_destroy(&this->non_full);
		condition_destroy(&this->non_empty);
	} else if (this->flags & CHANNEL_BLOCKING) {
		notice_destroy(this->rendezvous+1);
		notice_destroy(this->rendezvous+0);
	}
	lock_destroy(&this->entry);
	if (this->capacity > 1) {
		free(this->buffer);
		this->buffer = (Scalar*)0;
	}
}

static inline void
channel_close (Channel* this)
{
	this->flags |= CHANNEL_CLOSED;
	//?lock_acquire(&this->entry);
	if (this->occupation == 0) {
		this->flags |= CHANNEL_DRAINED;
	}
	//?lock_release(&this->entry);
}

static ALWAYS inline bool
channel_drained (Channel* this)
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

static inline int
channel_send (Channel* this, Scalar message)
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
		catch (notice_check(this->rendezvous+0));
		this->value = message;
		catch (notice_notify(this->rendezvous+1));
	} else if (this->capacity == 1) {
		assert(_channel_empty(this));
		this->value = message;
	} else {
		assert(this->capacity > 1);
		this->buffer[this->front] = message;
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

static inline int
channel_receive (Channel* this, Scalar* message)
{
	if (this->flags & CHANNEL_DRAINED) {
		if (message) *message = (Scalar)(Unsigned)0x0;
		return STATUS_SUCCESS;
	}

	int err;
	ENTER_CHANNEL_MONITOR (_channel_empty, this->non_empty)

	if (this->flags & CHANNEL_BLOCKING) {
		assert(this->capacity == 1);
		// protocol
		//    thread a: wait(0)-A-notify(1)
		//    thread b: notify(0)-wait(1)-B
		catch (notice_notify(this->rendezvous+0));
		catch (notice_check(this->rendezvous+1));
		if (message) *message = this->value;
	} else if (this->capacity == 1) {
		assert(_channel_full(this));
		if (message) *message = this->value;
	} else if (message) {
		assert(this->capacity > 1);
		register int back = this->front - this->occupation;
		back = (back >= 0) ? back : back+this->capacity;
		*message = this->buffer[back];
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

#undef catch
#undef ASSERT_CHANNEL_INVARIANT
#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
