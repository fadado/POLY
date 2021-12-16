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
	short permits;
	short waiting;
	cnd_t queue;
} Event;

static inline int  evt_init(Event* self);
static inline void evt_destroy(Event* self);
static inline int  evt_wait(Event* self, union lck_ptr lock);
static inline int  evt_signal(Event* self);
static inline int  evt_broadcast(Event* self);
static inline int  evt_wait_next(Event* self, union lck_ptr lock);
static inline int  evt_watch(Event* self, union lck_ptr lock, struct timespec* ts);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int evt_init(Event* self)
{
	self->waiting = self->permits = 0;
	return cnd_init(&self->queue);
}

static inline void evt_destroy(Event* self)
{
	assert(self->permits == 0);
	assert(self->waiting == 0);

	cnd_destroy(&self->queue);
}

static inline int evt_wait(Event* self, union lck_ptr lock)
{
	while (self->permits == 0) {
		++self->waiting;
		int err = cnd_wait(&self->queue, lock.mutex);
		--self->waiting;
		if (err != thrd_success) return err;
	}
	--self->permits;
	return thrd_success;
}

static inline int evt_wait_next(Event* self, union lck_ptr lock)
{
	do {
		++self->waiting;
		int err = cnd_wait(&self->queue, lock.mutex);
		--self->waiting;
		if (err != thrd_success) return err;
	} while (self->permits == 0);
	--self->permits;
	return thrd_success;
}

static inline int evt_watch(Event* self, union lck_ptr lock, struct timespec* ts)
{
	do {
		++self->waiting;
		int err = cnd_timedwait(&self->queue, lock.mutex, ts);
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
