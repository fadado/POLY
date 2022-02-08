#ifndef QUEUE_H
#define QUEUE_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"

////////////////////////////////////////////////////////////////////////
// Type Queue
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Queue {
	int    permits; // # of threads allowed to leave the queue
	int    waiting; // # of threads waiting in the queue
	mtx_t* mutex;   // monitor lock
	cnd_t  queue;   // monitor condition
} Queue;

static inline int  queue_broadcast(Queue* this);
static inline int  queue_check(Queue* this);
static inline void queue_destroy(Queue* this);
static inline bool queue_empty(Queue* this);
static inline int  queue_init(Queue* this, union Lock lock);
static inline int  queue_init2(Queue* q1, Queue* q2, union Lock lock);
static inline int  queue_notify(Queue* this);
static inline int  queue_wait(Queue* this);
static inline int  queue_wait_for(Queue* this, Time duration);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
static ALWAYS inline int _queue_length(Queue* this)
{ return this->waiting; }
*/

static ALWAYS inline bool queue_empty(Queue* this)
{ return this->waiting == 0; }

static inline int queue_init(Queue* this, union Lock lock)
{
	this->waiting = this->permits = 0;
	this->mutex = lock.mutex;
	return cnd_init(&this->queue);
}

static inline int queue_init2(Queue* q1, Queue* q2, union Lock lock)
{
	int err;
	if ((err=queue_init(q1, lock)) == STATUS_SUCCESS) {
		if ((err=queue_init(q2, lock)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			queue_destroy(q1);
		}
	}
	return err;
}

static inline void queue_destroy(Queue* this)
{
	assert(this->permits == 0);
	assert(this->waiting == 0);

	this->mutex = (mtx_t*)0;
	cnd_destroy(&this->queue);
}

static inline int queue_check(Queue* this)
{
	// until permits > 0
	while (this->permits == 0) {
		++this->waiting;
		int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	}
	--this->permits;
	return STATUS_SUCCESS;
}

static inline int queue_wait(Queue* this)
{
	do {
		++this->waiting;
		int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	// until permits > 0
	} while (this->permits == 0);
	--this->permits;
	return STATUS_SUCCESS;
}

static ALWAYS inline int queue_notify(Queue* this)
{
	++this->permits;
	return cnd_signal(&this->queue);
}

static ALWAYS inline int queue_broadcast(Queue* this)
{
	if (this->waiting > 0) {
		this->permits += this->waiting;
		return cnd_broadcast(&this->queue);
	}
}

static ALWAYS inline int queue_wait_for(Queue* this, Time duration)
{
	Time t = now();
	t += duration; // TIME_UTC based absolute calendar time point
	time_t s  = ns2s(t);
	long   ns = t - s2ns(s);
	return cnd_timedwait(&this->queue, this->mutex, &(struct timespec){.tv_sec=s, .tv_nsec=ns});
}

// vim:ai:sw=4:ts=4:syntax=cpp
#endif // QUEUE_H
