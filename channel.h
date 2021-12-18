/*
 * Channels of scalars
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

#include <stdbool.h>
#include <stdlib.h> // calloc

#include "scalar.h"
#include "rendezvous.h" // include event.h, lock.h

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	short flags;
	short capacity;
	short occupation;
	short front;
	union {
		Scalar* buffer; // for capacity > 1
		Scalar  value;  // for capacity == 1
	};
	Lock lock;
	union {
		// buffered channel
		struct {
			cnd_t non_empty;
			cnd_t non_full;
		};
		// blocking channel
		RendezVous rendezvous;
	};
} Channel;

static inline int  chn_init(Channel* self, unsigned capacity);
static inline void chn_destroy(Channel* self);
static inline int  chn_send_(Channel* self, Scalar x);
static inline int  chn_receive(Channel* self, Scalar* x);
static inline void chn_close(Channel* self);
static inline bool chn_exhaust(Channel* self);

// Accept any scalar type
#define chn_send(CHANNEL,EXPRESSION) chn_send_((CHANNEL), coerce((EXPRESSION)))

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

// Constants for flags
enum channel_flag {
	CHANNEL_BUFFERED    = (1<<0),
	CHANNEL_BLOCKING    = (1<<1),
	CHANNEL_CLOSED      = (1<<2),
	CHANNEL_EXHAUSTED   = (1<<3),
};

#ifdef DEBUG
#	define ASSERT_CHANNEL_INVARIANT\
		assert(0 <= self->occupation && self->occupation <= self->capacity);\
		if (self->capacity > 1) {\
			assert(0 <= self->front && self->front <  self->capacity);\
			assert(self->buffer != (Scalar*)0);\
		}\
		assert(!(_chn_empty(self) && _chn_full(self)));
#else
#	define ASSERT_CHANNEL_INVARIANT
#endif

// Same error management strategy in all this module
#define catch(X) if ((err=(X))!=thrd_success) goto onerror

//
// Predicates
//
static ALWAYS inline bool _chn_empty(Channel* self)
{
	return self->occupation == 0;
}

static ALWAYS inline bool _chn_full(Channel* self)
{
	return self->occupation == self->capacity;
}

//
// Channel life
//

static inline int chn_init(Channel* self, unsigned capacity)
{
	void d_lock(void)   { lck_destroy(&self->lock); }
	void d_empty(void)  { cnd_destroy(&self->non_empty); }
	void d_buffer(void) { free(self->buffer); }

	// cleanup thunks to call before return
	Thunk thunks[4] = { 0 };
	int thunk_index = 0;
#	define push(F) thunks[thunk_index++]=F

	int err;

	self->occupation = self->flags = 0;
	self->capacity = capacity;
	catch (lck_init(&self->lock));
	push(d_lock);

	switch (self->capacity) {
		case 0:
			catch (rv_init(&self->rendezvous));
			self->capacity = 1; // reset value to 1!
			self->flags |= CHANNEL_BLOCKING;
			break;
		default: // > 1
			self->front = 0;
			self->buffer = calloc(self->capacity, sizeof(Scalar));
			if (self->buffer == (Scalar*)0) {
				err = thrd_nomem;
				goto onerror;
			}
			push(d_buffer);
			fallthrough;
		case 1:
			catch (cnd_init(&self->non_empty));
			push(d_empty);
			catch (cnd_init(&self->non_full));
			self->flags |= CHANNEL_BUFFERED;
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	return thrd_success;
onerror:
#	undef push
	if (thunk_index > 0) {
		assert(thunk_index < sizeof(thunks)/sizeof(thunks[0]));
		Thunk* t = thunks;
		while (*t) { (*t++)(); }
	}
	return err;
}

static inline void chn_destroy(Channel* self)
{
	assert(_chn_empty(self)); // ???

	lck_destroy(&self->lock);
	if (self->flags & CHANNEL_BUFFERED) {
		cnd_destroy(&self->non_empty);
		cnd_destroy(&self->non_full);
	} else if (self->flags & CHANNEL_BLOCKING) {
		rv_destroy(&self->rendezvous);
	}
	if (self->capacity > 1) {
		free(self->buffer);
		self->buffer = (Scalar*)0; // sanitize
	}
}

static inline void chn_close(Channel* self)
{
	lck_acquire(&self->lock); // assume the mutex cannot fail...
	self->flags |= CHANNEL_CLOSED;
	if (self->occupation == 0) { // empty?
		self->flags |= CHANNEL_EXHAUSTED;
	}
	lck_release(&self->lock);
}

static ALWAYS inline bool chn_exhaust(Channel* self)
{
	lck_acquire(&self->lock); // assume the mutex cannot fail...
	bool b = self->flags & CHANNEL_EXHAUSTED;
	lck_release(&self->lock);
	return b;
}

//
// Monitor helpers
//
#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	if ((err=lck_acquire(&self->lock))!=thrd_success) {\
		return err;\
	}\
	if (self->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else while (PREDICATE(self)) {\
		if ((err=cnd_wait(&CONDITION, &self->lock.mutex))!=thrd_success) {\
			lck_destroy(&self->lock);\
			return err;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if (self->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else if ((err=cnd_signal(&CONDITION))!=thrd_success) {\
		lck_release(&self->lock);\
		return err;\
	}\
	if ((err=lck_release(&self->lock))!=thrd_success) {\
		return err;\
	}

static inline int chn_send_(Channel* self, Scalar x)
{
	int err;
	ENTER_CHANNEL_MONITOR (_chn_full, self->non_full)

	if (self->flags & CHANNEL_CLOSED) {
		panic("chn_send want to send an scalar to a closed channel");
	}

	if (self->flags & CHANNEL_BLOCKING) {
		assert(self->capacity == 1);
		// protocol
		//    thread a: wait(0)-A-signal(1)
		//    thread b: signal(0)-wait(1)-B
		catch (rv_wait(&self->rendezvous, 0, &self->lock));
		self->value = x;
		catch (rv_signal(&self->rendezvous, 1));
		//
	} else if (self->capacity == 1) {
		assert(_chn_empty(self));
		self->value = x;
	} else {
		assert(self->capacity > 1);
		self->buffer[self->front] = x;
		self->front = (self->front+1) % self->capacity;
	}
	++self->occupation;
	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (self->non_empty)

	return thrd_success;
onerror:
	lck_release(&self->lock);
	return err;
}

static inline int chn_receive(Channel* self, Scalar* x)
{
	int err;
	ENTER_CHANNEL_MONITOR (_chn_empty, self->non_empty)

	if (self->flags & CHANNEL_BLOCKING) {
		assert(self->capacity == 1);
		// protocol
		//    thread a: wait(0)-A-signal(1)
		//    thread b: signal(0)-wait(1)-B
		catch (rv_signal(&self->rendezvous, 0));
		catch (rv_wait(&self->rendezvous, 1, &self->lock));
		if (x) *x = self->value;
		//
	} else if (self->capacity == 1) {
		assert(_chn_full(self));
		if (x) *x = self->value;
	} else if (x) {
		assert(self->capacity > 1);
		int back = self->front - self->occupation;
		back = (back >= 0) ? back : back+self->capacity;
		*x = self->buffer[back];
	}
	--self->occupation;
	ASSERT_CHANNEL_INVARIANT
	if ((self->flags & CHANNEL_CLOSED) && self->occupation == 0) {
		self->flags |= CHANNEL_EXHAUSTED;
	}

	LEAVE_CHANNEL_MONITOR (self->non_full)

	return thrd_success;
onerror:
	lck_release(&self->lock);
	return err;
}

#undef catch

#undef ASSERT_CHANNEL_INVARIANT

#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
