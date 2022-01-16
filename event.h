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
	mtx_t* mutex;   // monitor lock
	cnd_t  queue;   // monitor condition
} Event;

static inline int  evt_broadcast(Event* this);
static inline void evt_destroy(Event* this);
static inline int  evt_init(Event* this, union lck_ptr lock);
static inline int  evt_init2(Event pair[2], union lck_ptr lock);
static inline int  evt_notify(Event* this);
static inline int  evt_stay(Event* this);
static inline int  evt_wait(Event* this);
static inline int  evt_wait_for(Event* this, unsigned long long nanoseconds);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int _evt_length(Event* this)
{ return this->waiting; }

static ALWAYS inline bool _evt_empty(Event* this)
{ return this->waiting == 0; }

static inline int evt_init(Event* this, union lck_ptr lock)
{
	this->waiting = this->permits = 0;
	this->mutex = lock.mutex;
	return cnd_init(&this->queue);
}

static inline int evt_init2(Event pair[2], union lck_ptr lock)
{
	int err;
	if ((err=evt_init(&pair[0], lock.mutex)) == STATUS_SUCCESS) {
		if ((err=evt_init(&pair[1], lock.mutex)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			evt_destroy(&pair[0]);
		}
	}
	return err;
}

static inline void evt_destroy(Event* this)
{
	assert(this->permits == 0);
	assert(this->waiting == 0);

	this->mutex = (mtx_t*)0;
	cnd_destroy(&this->queue);
}

static inline int evt_wait(Event* this)
{
	while (this->permits == 0) {
		++this->waiting;
		int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	}
	--this->permits;
	return STATUS_SUCCESS;
}

static inline int evt_stay(Event* this)
{
	do {
		++this->waiting;
		int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	} while (this->permits == 0);
	--this->permits;
	return STATUS_SUCCESS;
}

static ALWAYS inline int evt_notify(Event* this)
{
	++this->permits;
	return cnd_signal(&this->queue);
}

static ALWAYS inline int evt_broadcast(Event* this)
{
	if (this->waiting > 0) {
		this->permits += this->waiting;
		return cnd_broadcast(&this->queue);
	}
}

static ALWAYS inline int evt_wait_for(Event* this, unsigned long long nanoseconds)
{
	time_t s  = ns2s(nanoseconds);
	long   ns = nanoseconds - s2ns(s);
	return cnd_timedwait(&this->queue, this->mutex,
						 &(struct timespec){.tv_sec=s, .tv_nsec=ns});
}

#endif // EVENT_H

// vim:ai:sw=4:ts=4:syntax=cpp
