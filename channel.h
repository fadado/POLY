/*
 * Channels of scalars
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stdlib.h> // calloc
#include <threads.h>
#include <stdatomic.h>

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

#include "scalar.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
////////////////////////////////////////////////////////////////////////

//
typedef struct Channel {
	// channel state/properties (see enum channel_flag)
	atomic_ushort flags;
	// channel size and occupation
	short size;
	short count;
	// circular buffer
	short front; // rear is a function of count and front
	union {
		Scalar* buffer; // for size > 1
		Scalar  value;  // optimize for size == 1
	};
	// monitor
	mtx_t lock;
	cnd_t non_empty;
	cnd_t non_full;
	// rendez-vous
	cnd_t barrier[2];
	bool  waking[2];
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
const char* RV[2] = {
	[HOLA_DON_PEPITO] = "HOLA_DON_PEPITO",
	[HOLA_DON_JOSE] = "HOLA_DON_JOSE",
};

//
#ifdef DEBUG
#	define ASSERT_CHANNEL_INVARIANT\
		assert(0 <= self->count && self->count <= self->size);\
		assert(0 <= self->front && self->front <  self->size);\
		assert(self->size < 2 || self->buffer != (Scalar*)0);\
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
#if 1
	return self->flags & CHANNEL_EXHAUSTED;
#else
	mtx_lock(&self->lock);
	bool exhaust = (self->flags & CHANNEL_CLOSED) && _chn_empty(self);
	mtx_unlock(&self->lock);
	return exhaust;
#endif
}

//
// Channel life
//

/*
 *
 */
static inline int chn_init(Channel* self, unsigned capacity)
{
	int err;
	self->count = self->flags = 0;
	self->size = capacity;

	switch (self->size) {
		case 0:
			self->waking[HOLA_DON_PEPITO] = self->waking[HOLA_DON_JOSE] = false;
			if ((err=cnd_init(&self->barrier[HOLA_DON_PEPITO])) != thrd_success) {
				return err;
			}
			if ((err=cnd_init(&self->barrier[HOLA_DON_JOSE])) != thrd_success) {
				cnd_destroy(&self->barrier[HOLA_DON_PEPITO]);
				return err;
			}
			self->size = 1; // reset value to 1!
			self->flags |= CHANNEL_BLOCKING;
			break;
		case 1:
			self->flags |= CHANNEL_BUFFERED;
			break;
		default: // > 1
			self->front = 0;
			self->buffer = calloc(self->size, sizeof(Scalar));
			if (self->buffer == (Scalar*)0) return thrd_nomem;
			self->flags |= CHANNEL_BUFFERED;
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	// initialize mutex and conditions
	int eN=0;
#	define catch(X)	if ((++eN,err=(X))!=thrd_success) goto onerror

	catch (mtx_init(&self->lock, mtx_plain)); // eN == 1
	catch (cnd_init(&self->non_empty));       // eN == 2
	catch (cnd_init(&self->non_full));        // eN == 3

	return thrd_success;
onerror:
#	undef catch
	assert(err != thrd_success);

	assert(1 <= eN && eN <= 3);
	switch (eN) {
		case 3: cnd_destroy(&self->non_empty);
		case 2: mtx_destroy(&self->lock);
		case 1: if (self->size > 1) free(self->buffer);
	}
	if (self->flags & CHANNEL_BLOCKING) {
		cnd_destroy(&self->barrier[HOLA_DON_PEPITO]);
		cnd_destroy(&self->barrier[HOLA_DON_JOSE]);
	}
	return err;
}

/*
 *
 */
static inline void chn_destroy(Channel* self)
{
	assert(_chn_empty(self)); // ???

	mtx_destroy(&self->lock);
	cnd_destroy(&self->non_empty);
	cnd_destroy(&self->non_full);

	if (self->flags & CHANNEL_BLOCKING) {
		cnd_destroy(&self->barrier[HOLA_DON_PEPITO]);
		cnd_destroy(&self->barrier[HOLA_DON_JOSE]);
	} else if (self->size == 1) {
		; // NOP
	} else { // self->size > 1
		free(self->buffer);
		self->buffer = (Scalar*)0; // sanitize
	}
}

/*
 *
 */
static ALWAYS inline void chn_close(Channel* self)
{
#if 1
	atomic_fetch_or(&self->flags, CHANNEL_CLOSED);
#else
	mtx_lock(&self->lock);
	self->flags |= CHANNEL_CLOSED;
	mtx_unlock(&self->lock);
#endif
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

#define WAIT(Ix)\
	warn("WAIT   @ %s",RV[Ix]);\
	while (!self->waking[Ix]) {\
		err_=cnd_wait(&self->barrier[Ix], &self->lock);\
		if (err_!=thrd_success) {mtx_unlock(&self->lock);return err_;}\
	}\
	self->waking[Ix]=false;\
	assert(self->waking[Ix] >= 0);

#define SIGNAL(Ix)\
	warn("SIGNAL @ %s", RV[Ix]);\
	self->waking[Ix]=true;\
	err_=cnd_signal(&self->barrier[Ix]);\
	if (err_!=thrd_success) {mtx_unlock(&self->lock);return err_;}

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
		// assert(not_implemented);
		assert(self->size == 1);
		//BUG: assert(_chn_empty(self));

		WAIT (HOLA_DON_PEPITO)

		assert(!_chn_full(self));
		self->value = x;

		SIGNAL (HOLA_DON_JOSE)
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
		// assert(not_implemented);
		assert(self->size == 1);
		//BUG: assert(_chn_full(self));

		SIGNAL (HOLA_DON_PEPITO)
		WAIT   (HOLA_DON_JOSE)

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

#undef WAIT
#undef SIGNAL

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
