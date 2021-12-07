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

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

#include "scalar.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	// some properties (see enum channel_flag)
	short unsigned flags;
	// circular buffer
	short size;
	short count;
	short front;
	union {
		Scalar* buffer; // for size > 1
		Scalar  value;  // optimize for size == 1
	};
	mtx_t lock;
	cnd_t non_empty;
	cnd_t non_full;
	// Rendez-vous
	cnd_t barrier[2];
	int   waking[2];
} Channel;

// Constants for flags
enum channel_flag {
	CHANNEL_BUFFERED    = (1<<0),
	CHANNEL_BLOCKING    = (1<<1),
	CHANNEL_CLOSED      = (1<<2),
};

// Constants for rendez-vous
enum {
	_CHN_RECEIVER, _CHN_SENDER
};

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

static ALWAYS inline bool _chn_flag(Channel* self, enum channel_flag flag)
{
	return (self->flags & flag) == flag;
}

/*
 *
 */
static ALWAYS inline bool chn_exhaust(Channel* self)
{
	mtx_lock(&self->lock);
	bool exhaust = _chn_flag(self, CHANNEL_CLOSED) && _chn_empty(self);
	mtx_unlock(&self->lock);
	return exhaust;
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
			self->waking[_CHN_RECEIVER] = self->waking[_CHN_SENDER] = 0;
			if ((err=cnd_init(&self->barrier[_CHN_RECEIVER])) != thrd_success) {
				return err;
			}
			if ((err=cnd_init(&self->barrier[_CHN_SENDER])) != thrd_success) {
				cnd_destroy(&self->barrier[_CHN_RECEIVER]);
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
			self->flags |= CHANNEL_BUFFERED;
			self->buffer = calloc(self->size, sizeof(Scalar));
			if (self->buffer == (Scalar*)0) return thrd_nomem;
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
	if (_chn_flag(self, CHANNEL_BLOCKING)) {
		cnd_destroy(&self->barrier[_CHN_RECEIVER]);
		cnd_destroy(&self->barrier[_CHN_SENDER]);
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

	if (_chn_flag(self, CHANNEL_BLOCKING)) {
		cnd_destroy(&self->barrier[_CHN_RECEIVER]);
		cnd_destroy(&self->barrier[_CHN_SENDER]);
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
	mtx_lock(&self->lock);
	self->flags |= CHANNEL_CLOSED;
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
	while (PREDICATE(self)) {\
		if ((err_=cnd_wait(&CONDITION, &self->lock))!=thrd_success) {\
			mtx_destroy(&self->lock);\
			return err_;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if ((err_=cnd_signal(&CONDITION))!=thrd_success) {\
		mtx_unlock(&self->lock);\
		return err_;\
	}\
	if ((err_=mtx_unlock(&self->lock))!=thrd_success) {\
		return err_;\
	}

#define CHECK_CHANNEL_MONITOR(E)\
	if ((E)!=thrd_success) {\
		mtx_unlock(&self->lock);\
		return (E);\
	}

/*
 *
 */
static inline int chn_send_(Channel* self, Scalar x)
{
	ENTER_CHANNEL_MONITOR (_chn_full, self->non_full)

	if (_chn_flag(self, CHANNEL_CLOSED)) {
		panic("chn_send want to send an scalar to a closed channel");
	}

	if (_chn_flag(self, CHANNEL_BLOCKING)) {
		// assert(not_implemented);
		assert(self->size == 1);
		assert(_chn_empty(self));

		int err;
		do {
			err = cnd_wait(&self->barrier[_CHN_RECEIVER], &self->lock);
			CHECK_CHANNEL_MONITOR (err)
		} while (!self->waking[_CHN_RECEIVER]);
		--self->waking[_CHN_RECEIVER];
		assert(self->waking[_CHN_RECEIVER] >= 0);

		assert(!_chn_full(self));
		self->value = x;

		++self->waking[_CHN_SENDER];
		err = cnd_signal(&self->barrier[_CHN_SENDER]);
		CHECK_CHANNEL_MONITOR (err)
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

	if (_chn_flag(self, CHANNEL_BLOCKING)) {
		// assert(not_implemented);
		assert(self->size == 1);
		assert(_chn_full(self));

		++self->waking[_CHN_RECEIVER];
		int err = cnd_signal(&self->barrier[_CHN_RECEIVER]);
		CHECK_CHANNEL_MONITOR (err)

		do {
			err = cnd_wait(&self->barrier[_CHN_SENDER], &self->lock);
			CHECK_CHANNEL_MONITOR (err)
		} while (!self->waking[_CHN_SENDER]);
		--self->waking[_CHN_SENDER];
		assert(self->waking[_CHN_SENDER] >= 0);

		assert(!_chn_empty(self));
		if (x) *x = self->value;
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

	LEAVE_CHANNEL_MONITOR (self->non_full)

	return thrd_success;
}

#undef ALWAYS
#undef ASSERT_CHANNEL_INVARIANT
#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
