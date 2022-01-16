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
#include "event.h"
#include "sign.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	int flags;
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
			Sign non_empty;
			Sign non_full;
		};
		// blocking channel
		Event rendezvous[2];
	};
} Channel;

static inline void chn_close(Channel* this);
static inline void chn_destroy(Channel* this);
static inline bool chn_drained(Channel* this);
static inline int  chn_init(Channel* this, unsigned capacity);
static inline int  chn_receive(Channel* this, Scalar* x);
static inline int  chn_send_(Channel* this, Scalar x);

// Accept any scalar type
#define chn_send(CHANNEL,EXPRESSION) chn_send_((CHANNEL), coerce(EXPRESSION))

// handy macro
#define spawn_filter(I,O,T,...)\
	tsk_spawn(T,&(struct T){.input=I,.output=O __VA_OPT__(,)__VA_ARGS__ })

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
		assert(!(_chn_empty(this) && _chn_full(this)));
#else
#	define ASSERT_CHANNEL_INVARIANT
#endif

// Same error management strategy in all this module
#define catch(X) if ((err=(X))!=STATUS_SUCCESS) goto onerror

//
// Predicates
//
static ALWAYS inline bool _chn_empty(Channel* this)
{
	return this->occupation == 0;
}

static ALWAYS inline bool _chn_full(Channel* this)
{
	return this->occupation == this->capacity;
}

//
// Channel life
//

static inline int chn_init(Channel* this, unsigned capacity)
{
	void d_lock(void)   { lck_destroy(&this->entry); }
	void d_empty(void)  { sign_destroy(&this->non_empty); }
	void d_buffer(void) { free(this->buffer); }

	// cleanup thunks to call before return
	void(*thunks[4])(void) = { 0 };
	int thunk_index = 0;
#	define push(F) thunks[thunk_index++]=F

	int err;

	this->occupation = this->flags = 0;
	this->capacity = capacity;
	catch (lck_init(&this->entry));
	push(d_lock);

	switch (this->capacity) {
		case 0:
			catch (evt_init2(this->rendezvous, &this->entry));
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
			push(d_buffer);
			fallthrough;
		case 1:
			catch (sign_init(&this->non_empty));
			push(d_empty);
			catch (sign_init(&this->non_full));
			this->flags |= CHANNEL_BUFFERED;
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	return STATUS_SUCCESS;
onerror:
#	undef push
	if (thunk_index > 0) {
		assert(thunk_index < sizeof(thunks)/sizeof(thunks[0]));
		int i;
		for (i=0; thunks[i] != (void(*)(void))0; ++i) ;
		for (--i; i >= 0; --i) (*thunks[i])();
	}
	return err;
}

static inline void chn_destroy(Channel* this)
{
	assert(_chn_empty(this));

	if (this->flags & CHANNEL_BUFFERED) {
		sign_destroy(&this->non_full);
		sign_destroy(&this->non_empty);
	} else if (this->flags & CHANNEL_BLOCKING) {
		evt_destroy(&this->rendezvous[1]);
		evt_destroy(&this->rendezvous[0]);
	}
	lck_destroy(&this->entry);
	if (this->capacity > 1) {
		free(this->buffer);
		this->buffer = (Scalar*)0;
	}
}

static inline void chn_close(Channel* this)
{
	lck_acquire(&this->entry);
	this->flags |= CHANNEL_CLOSED;
	if (this->occupation == 0) {
		this->flags |= CHANNEL_DRAINED;
	}
	lck_release(&this->entry);
}

static ALWAYS inline bool chn_drained(Channel* this)
{
	lck_acquire(&this->entry);
	bool b = this->flags & CHANNEL_DRAINED;
	lck_release(&this->entry);
	return b;
}

//
// Monitor helpers
//
#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	if ((err=lck_acquire(&this->entry))!=STATUS_SUCCESS) {\
		return err;\
	}\
	if (this->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else while (PREDICATE(this)) {\
		if ((err=sign_wait(&CONDITION, &this->entry.mutex))!=STATUS_SUCCESS) {\
			lck_destroy(&this->entry);\
			return err;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if (this->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else if ((err=sign_give(&CONDITION))!=STATUS_SUCCESS) {\
		lck_release(&this->entry);\
		return err;\
	}\
	if ((err=lck_release(&this->entry))!=STATUS_SUCCESS) {\
		return err;\
	}

static inline int chn_send_(Channel* this, Scalar x)
{
	int err;
	ENTER_CHANNEL_MONITOR (_chn_full, this->non_full)

	if (this->flags & CHANNEL_CLOSED) {
		panic("chn_send want to send an scalar to a closed channel");
	}

	if (this->flags & CHANNEL_BLOCKING) {
		assert(this->capacity == 1);
		// protocol
		//    thread a: wait(0)-A-notify(1)
		//    thread b: notify(0)-wait(1)-B
		catch (evt_wait(&this->rendezvous[0]));
		this->value = x;
		catch (evt_notify(&this->rendezvous[1]));
	} else if (this->capacity == 1) {
		assert(_chn_empty(this));
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
	lck_release(&this->entry);
	return err;
}

static inline int chn_receive(Channel* this, Scalar* x)
{
	if (this->flags & CHANNEL_DRAINED) {
		if (x) x->word = 0x0;
		return STATUS_SUCCESS;
	}

	int err;
	ENTER_CHANNEL_MONITOR (_chn_empty, this->non_empty)

	if (this->flags & CHANNEL_BLOCKING) {
		assert(this->capacity == 1);
		// protocol
		//    thread a: wait(0)-A-notify(1)
		//    thread b: notify(0)-wait(1)-B
		catch (evt_notify(&this->rendezvous[0]));
		catch (evt_wait(&this->rendezvous[1]));
		if (x) *x = this->value;
		//
	} else if (this->capacity == 1) {
		assert(_chn_full(this));
		if (x) *x = this->value;
	} else if (x) {
		assert(this->capacity > 1);
		int back = this->front - this->occupation;
		back = (back >= 0) ? back : back+this->capacity;
		*x = this->buffer[back];
	}
	--this->occupation;
	ASSERT_CHANNEL_INVARIANT
	if ((this->flags & CHANNEL_CLOSED) && this->occupation == 0) {
		this->flags |= CHANNEL_DRAINED;
	}

	LEAVE_CHANNEL_MONITOR (this->non_full)

	return STATUS_SUCCESS;
onerror:
	lck_release(&this->entry);
	return err;
}

////////////////////////////////////////////////////////////////////////
// Dynamic allocation
////////////////////////////////////////////////////////////////////////

static inline Channel* chn_alloc(int capacity)
{
	Channel* channel = calloc(capacity ? capacity : 1, sizeof(Channel));
	if (channel == (Channel*)0) {
		panic("chn_alloc out of memory");
	}
	int err = chn_init(channel, capacity);
	if (err != STATUS_SUCCESS) {
		free(channel);
		panic("chn_alloc cannot initialize channel");
	}
	return channel;
}

static inline void chn_free(Channel* channel)
{
	if (channel) {
		chn_destroy(channel);
		free(channel);
	}
}

#undef catch
#undef ASSERT_CHANNEL_INVARIANT
#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
