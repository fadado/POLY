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
#include "rendezvous.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	// channel state/properties (see enum channel_flag)
	short flags;
	// channel size and occupation
	short size;
	short count;
	union {
		Scalar* buffer; // for size > 1
		Scalar  value;  // for size == 1
	};
	// monitor
	mtx_t mutex;
	union {
		// buffered channels
		struct {
			short front; // rear is a function of count and front
			cnd_t non_empty;
			cnd_t non_full;
		};
		// blocking channels
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
		assert(0 <= self->count && self->count <= self->size);\
		if (self->size > 1) {\
			assert(0 <= self->front && self->front <  self->size);\
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
	return self->count == 0;
}

static ALWAYS inline bool _chn_full(Channel* self)
{
	return self->count == self->size;
}

//
// Channel life
//

static inline int chn_init(Channel* self, unsigned capacity)
{
	void d_mutex(void)  { mtx_destroy(&self->mutex); }
	void d_empty(void)  { cnd_destroy(&self->non_empty); }
	void d_buffer(void) { free(self->buffer); }

	// cleanup thunks to call before return
	Thunk thunks[4] = { 0 };
	int thunk_index = 0;
#	define push(F) thunks[thunk_index++]=F

	int err;

	self->count = self->flags = 0;
	self->size = capacity;
	catch (mtx_init(&self->mutex, mtx_plain));
	push(d_mutex);

	switch (self->size) {
		case 0:
			catch (rv_init(&self->rendezvous));
			self->size = 1; // reset value to 1!
			self->flags |= CHANNEL_BLOCKING;
			break;
		default: // > 1
			self->front = 0;
			self->buffer = calloc(self->size, sizeof(Scalar));
			if (self->buffer == (Scalar*)0) {
				err = thrd_nomem;
				goto onerror;
			}
			push(d_buffer);
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

	mtx_destroy(&self->mutex);
	if (self->flags & CHANNEL_BUFFERED) {
		cnd_destroy(&self->non_empty);
		cnd_destroy(&self->non_full);
	} else if (self->flags & CHANNEL_BLOCKING) {
		rv_destroy(&self->rendezvous);
	}
	if (self->size > 1) {
		free(self->buffer);
		self->buffer = (Scalar*)0; // sanitize
	}
}

static inline void chn_close(Channel* self)
{
	mtx_lock(&self->mutex); // assume the mutex cannot fail...
	self->flags |= CHANNEL_CLOSED;
	if (self->size == 0) { // empty?
		self->flags |= CHANNEL_EXHAUSTED;
	}
	mtx_unlock(&self->mutex);
}

static ALWAYS inline bool chn_exhaust(Channel* self)
{
	mtx_lock(&self->mutex); // assume the mutex cannot fail...
	bool b = self->flags & CHANNEL_EXHAUSTED;
	mtx_unlock(&self->mutex);
	return b;
}

//
// Monitor helpers
//
#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	if ((err=mtx_lock(&self->mutex))!=thrd_success) {\
		return err;\
	}\
	if (self->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else while (PREDICATE(self)) {\
		if ((err=cnd_wait(&CONDITION, &self->mutex))!=thrd_success) {\
			mtx_destroy(&self->mutex);\
			return err;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if (self->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else if ((err=cnd_signal(&CONDITION))!=thrd_success) {\
		mtx_unlock(&self->mutex);\
		return err;\
	}\
	if ((err=mtx_unlock(&self->mutex))!=thrd_success) {\
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
		assert(self->size == 1);
		// protocol
		//    thread a: wait(0)-A-signal(1)
		//    thread b: signal(0)-wait(1)-B
		catch (rv_wait(&self->rendezvous, 0, &self->mutex));
		self->value = x;
		catch (rv_signal(&self->rendezvous, 1));
		//
	} else if (self->size == 1) {
		assert(_chn_empty(self));
		self->value = x;
	} else {
		assert(self->size > 1);
		self->buffer[self->front] = x;
		self->front = (self->front+1) % self->size;
	}
	++self->count;
	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (self->non_empty)

	return thrd_success;
onerror:
	mtx_unlock(&self->mutex);
	return err;
}

static inline int chn_receive(Channel* self, Scalar* x)
{
	int err;
	ENTER_CHANNEL_MONITOR (_chn_empty, self->non_empty)

	if (self->flags & CHANNEL_BLOCKING) {
		assert(self->size == 1);
		// protocol
		//    thread a: wait(0)-A-signal(1)
		//    thread b: signal(0)-wait(1)-B
		catch (rv_signal(&self->rendezvous, 0));
		catch (rv_wait(&self->rendezvous, 1, &self->mutex));
		if (x) *x = self->value;
		//
	} else if (self->size == 1) {
		assert(_chn_full(self));
		if (x) *x = self->value;
	} else if (x) {
		assert(self->size > 1);
		int back = self->front - self->count;
		back = (back >= 0) ? back : back+self->size;
		*x = self->buffer[back];
	}
	--self->count;
	ASSERT_CHANNEL_INVARIANT
	if ((self->flags & CHANNEL_CLOSED) && self->count == 0) {
		self->flags |= CHANNEL_EXHAUSTED;
	}

	LEAVE_CHANNEL_MONITOR (self->non_full)

	return thrd_success;
onerror:
	mtx_unlock(&self->mutex);
	return err;
}

#undef catch

#undef ASSERT_CHANNEL_INVARIANT

#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
