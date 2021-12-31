/*
 * Event
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef EVENT_H
#define EVENT_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"

////////////////////////////////////////////////////////////////////////
// Type Event
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Event {
	int    permits; // # of threads allowed to leave the queue
	int    waiting; // # of threads waiting in the queue
	mtx_t* mutex;
	cnd_t  queue;
} Event;

static inline int  evt_init(Event* self, union lck_ptr lock);
static inline void evt_destroy(Event* self);
static inline int  evt_wait(Event* self);
static inline int  evt_signal(Event* self);
static inline int  evt_broadcast(Event* self);
static inline int  evt_stay(Event* self);
static inline int  evt_watch(Event* self, unsigned long long nanoseconds);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int _evt_length(Event* self)
{ return self->waiting; }

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

	self->mutex = (mtx_t*)0; // sanitize
	cnd_destroy(&self->queue);
}

static inline int evt_wait(Event* self)
{
	while (self->permits == 0) {
		++self->waiting;
		int err = cnd_wait(&self->queue, self->mutex);
		--self->waiting;
		if (err != STATUS_SUCCESS) return err;
	}
	--self->permits;
	return STATUS_SUCCESS;
}

static inline int evt_stay(Event* self)
{
	do {
		++self->waiting;
		int err = cnd_wait(&self->queue, self->mutex);
		--self->waiting;
		if (err != STATUS_SUCCESS) return err;
	} while (self->permits == 0);
	--self->permits;
	return STATUS_SUCCESS;
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

static ALWAYS inline int evt_watch(Event* self, unsigned long long nanoseconds)
{
	time_t s = nanoseconds/1000000000ull;
	long   n = nanoseconds%1000000000ull;
	return cnd_timedwait(&self->queue, self->mutex,
						 &(struct timespec){.tv_sec=s, .tv_nsec=n});
}

#endif // EVENT_H

// vim:ai:sw=4:ts=4:syntax=cpp
