/*
 * Event (wrapper to cnd_t)
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef EVENT_H
#define EVENT_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

#include "lock.h"

////////////////////////////////////////////////////////////////////////
// Type Event
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Event {
	short  permits; // # of threads allowed to leave the queue
	short  waiting; // # of threads waiting in the queue
	mtx_t* mutex;
	cnd_t  queue;
} Event;

static inline int  evt_init(Event* self, union lck_ptr lock);
static inline void evt_destroy(Event* self);
static inline int  evt_wait(Event* self);
static inline int  evt_signal(Event* self);
static inline int  evt_broadcast(Event* self);
static inline int  evt_stay(Event* self);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int evt_init(Event* self, union lck_ptr lock)
{
	self->waiting = self->permits = 0;
	self->mutex = lock.mutex;
	return cnd_init(&self->queue);
}

static inline void evt_destroy(Event* self)
{
	assert(self->permits == 0);
	assert(self->waiting == 0);

	self->mutex = (mtx_t*)0; // sanitze
	cnd_destroy(&self->queue);
}

static inline int evt_wait(Event* self)
{
	while (self->permits == 0) {
		++self->waiting;
		int err = cnd_wait(&self->queue, self->mutex);
		--self->waiting;
		if (err != thrd_success) return err;
	}
	--self->permits;
	return thrd_success;
}

static inline int evt_stay(Event* self)
{
	do {
		++self->waiting;
		int err = cnd_wait(&self->queue, self->mutex);
		--self->waiting;
		if (err != thrd_success) return err;
	} while (self->permits == 0);
	--self->permits;
	return thrd_success;
}

static ALWAYS inline int evt_signal(Event* self)
{
	++self->permits;
	return cnd_signal(&self->queue);
}

static ALWAYS inline int evt_broadcast(Event* self)
{
	if (self->waiting > 0) {
		self->permits += self->waiting;
		return cnd_broadcast(&self->queue);
	}
}

#endif // EVENT_H

// vim:ai:sw=4:ts=4:syntax=cpp