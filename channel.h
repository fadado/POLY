/*
 * Channels of scalars
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

#include <stdbool.h>
#include <stdlib.h> // calloc
#include <threads.h>

#include "scalar.h"
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
////////////////////////////////////////////////////////////////////////

//
typedef struct Channel {
	// channel state/properties (see enum channel_flag)
	short flags;
	// channel size and occupation
	short size;
	short count;
	union {
		Scalar* buffer; // for size > 1
		Scalar  value;  // optimize for size == 1
	};
	// monitor
	mtx_t lock;
	union {
		// buffered channels
		struct {
			short front; // rear is a function of count and front
			cnd_t non_empty;
			cnd_t non_full;
		};
		// blocking channels
		Event rendezvous[2];
	};
} Channel;

// Constants for flags
enum channel_flag {
	CHANNEL_BUFFERED    = (1<<0),
	CHANNEL_BLOCKING    = (1<<1),
	CHANNEL_CLOSED      = (1<<2),
	CHANNEL_EXHAUSTED   = (1<<3),
};

// Constants for rendez-vous
#define HOLA_DON_PEPITO 0
#define HOLA_DON_JOSE   1

#ifdef DEBUG
const char* _RV_[2] = {
	[HOLA_DON_PEPITO] = "HOLA_DON_PEPITO",
	[HOLA_DON_JOSE]   = "HOLA_DON_JOSE",
};
#endif

//
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

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

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

#define ALWAYS __attribute__((always_inline))

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

/*
 *
 */
static ALWAYS inline bool chn_exhaust(Channel* self)
{
	mtx_lock(&self->lock); // assume the lock cannot fail...
	bool b = self->flags & CHANNEL_EXHAUSTED;
	mtx_unlock(&self->lock);
	return b;
}

//
// Channel life
//

//#define rv_init(R_V)\

/*
 *
 */
static inline int chn_init(Channel* self, unsigned capacity)
{
#	define catch(X)	if ((err=(X))!=thrd_success) goto onerror

	void d_mutex(void)  { mtx_destroy(&self->lock); }
	void d_empty(void)  { cnd_destroy(&self->non_empty); }
	void d_pepito(void) { evt_destroy(&self->rendezvous[HOLA_DON_PEPITO]); }
	void d_buffer(void) { free(self->buffer); }

	// cleanup thunks to call before return
	Thunk thunks[4] = { 0 };
	int thunk_index = 0;
#	define push(F) thunks[thunk_index++]=F

	int err;

	self->count = self->flags = 0;
	self->size = capacity;
	catch (mtx_init(&self->lock, mtx_plain));
	push(d_mutex);

	switch (self->size) {
		case 0:
			catch (evt_init(&self->rendezvous[HOLA_DON_PEPITO]));
			push(d_pepito);
			catch (evt_init(&self->rendezvous[HOLA_DON_JOSE]));
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
#	undef catch
#	undef push
	assert(thunk_index > 0);
	assert(thunk_index < sizeof(thunks)/sizeof(thunks[0]));
	Thunk* t = thunks;
	while (*t) { (*t++)(); }
	return err;
}

/*
 *
 */
#define rv_destroy(R_V)\
		evt_destroy(&R_V[0]);\
		evt_destroy(&R_V[1])

static inline void chn_destroy(Channel* self)
{
	assert(_chn_empty(self)); // ???

	mtx_destroy(&self->lock);
	if (self->flags & CHANNEL_BUFFERED) {
		cnd_destroy(&self->non_empty);
		cnd_destroy(&self->non_full);
	} else if (self->flags & CHANNEL_BLOCKING) {
		rv_destroy(self->rendezvous);
	}
	if (self->size > 1) {
		free(self->buffer);
		self->buffer = (Scalar*)0; // sanitize
	}
}

/*
 *
 */
static ALWAYS inline void chn_close(Channel* self)
{
	mtx_lock(&self->lock); // assume the lock cannot fail...
	self->flags |= CHANNEL_CLOSED;
	if (self->size == 0) { // empty?
		self->flags |= CHANNEL_EXHAUSTED;
	}
	mtx_unlock(&self->lock);
}

//
// Monitor helpers
//
#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	int err_;\
	if ((err_=mtx_lock(&self->lock))!=thrd_success) {\
		return err_;\
	}\
	if (self->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else while (PREDICATE(self)) {\
		if ((err_=cnd_wait(&CONDITION, &self->lock))!=thrd_success) {\
			mtx_destroy(&self->lock);\
			return err_;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if (self->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else if ((err_=cnd_signal(&CONDITION))!=thrd_success) {\
		mtx_unlock(&self->lock);\
		return err_;\
	}\
	if ((err_=mtx_unlock(&self->lock))!=thrd_success) {\
		return err_;\
	}

#define rv_wait(R_V,Ix) do {\
	trace("WAIT   @ %s",_RV_[Ix]);\
	err_=evt_wait(&R_V[Ix], &self->lock);\
	if (err_!=thrd_success) {mtx_unlock(&self->lock);return err_;}\
} while (0)

#define rv_signal(R_V,Ix) do {\
	trace("SIGNAL @ %s", _RV_[Ix]);\
	err_=evt_signal(&R_V[Ix]);\
	if (err_!=thrd_success) {mtx_unlock(&self->lock);return err_;}\
} while (0)

/*
 *
 */
static inline int chn_send_(Channel* self, Scalar x)
{
	ENTER_CHANNEL_MONITOR (_chn_full, self->non_full)

	if (self->flags & CHANNEL_CLOSED) {
		panic("chn_send want to send an scalar to a closed channel");
	}

	if (self->flags & CHANNEL_BLOCKING) {
		assert(self->size == 1);
		//BUG:assert(_chn_empty(self));

		rv_wait(self->rendezvous, HOLA_DON_PEPITO);

		assert(!_chn_full(self));
		self->value = x;

		rv_signal(self->rendezvous, HOLA_DON_JOSE);
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
}

/*
 *
 */
static inline int chn_receive(Channel* self, Scalar* x)
{
	ENTER_CHANNEL_MONITOR (_chn_empty, self->non_empty)

	if (self->flags & CHANNEL_BLOCKING) {
		assert(self->size == 1);
		//BUG:assert(_chn_empty(self));

		rv_signal(self->rendezvous, HOLA_DON_PEPITO);
		rv_wait(self->rendezvous, HOLA_DON_JOSE);

		assert(!_chn_empty(self));
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
}

#undef HOLA_DON_PEPITO
#undef HOLA_DON_JOSE

#undef ALWAYS

#undef ASSERT_CHANNEL_INVARIANT

#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#undef rv_destroy
#undef rv_wait
#undef rv_signal

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
