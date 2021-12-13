/*
 * Events
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef EVENT_H
#define EVENT_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

////////////////////////////////////////////////////////////////////////
// Type Event
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Event {
	int   state;
	cnd_t queue;
} Event;

static inline int  evt_init(Event* self);
static inline void evt_destroy(Event* self);
static inline int  evt_wait(Event* self, mtx_t* mutex);
static inline int  evt_wait_after(Event* self, mtx_t* mutex);
static inline int  evt_signal(Event* self);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int evt_init(Event* self)
{
	self->state = 0;
	return cnd_init(&self->queue);
}

static inline void evt_destroy(Event* self)
{
	cnd_destroy(&self->queue);
}

static inline int evt_wait(Event* self, mtx_t* mutex)
{
	// assume `mutex` is locked
	while (self->state == 0) {
		int err = cnd_wait(&self->queue, mutex);
		if (err != thrd_success) return err;
	}
	--self->state;
	return thrd_success;
}

static inline int evt_wait_after(Event* self, mtx_t* mutex)
{
	// assume `mutex` is locked
	do {
		int err = cnd_wait(&self->queue, mutex);
		if (err != thrd_success) return err;
	} while (self->state == 0);
	--self->state;
	return thrd_success;
}

static ALWAYS inline int evt_signal(Event* self)
{
	++self->state;
	return cnd_signal(&self->queue);
}

#endif // EVENT_H

// vim:ai:sw=4:ts=4:syntax=cpp
