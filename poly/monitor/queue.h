#ifndef QUEUE_H
#define QUEUE_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Queue {
	unsigned permits; // # of threads allowed to leave the queue
	unsigned waiting; // # of threads waiting in the queue
	mtx_t*   mutex;   // monitor lock
	cnd_t    queue;   // monitor condition
} Queue;

static inline int  queue_broadcast(Queue* this);
static inline int  queue_check(Queue* this);
static inline void queue_destroy(Queue* this);
static inline bool queue_empty(Queue* this);
static inline int  queue_init(Queue* this, union Lock lock);
static inline int  queue_notify(Queue* this);
static inline int  queue_wait(Queue* this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
static ALWAYS inline int
_queue_length (Queue* this)
{
	return this->waiting;
}
*/

static ALWAYS inline bool
queue_empty (Queue* this)
{
	return this->waiting == 0;
}

static inline int
queue_init (Queue* this, union Lock lock)
{
	this->waiting = this->permits = 0;
	this->mutex = lock.mutex;
	return cnd_init(&this->queue);
}

static inline void
queue_destroy (Queue* this)
{
	assert(this->permits == 0);
	assert(this->waiting == 0);

	this->mutex = (mtx_t*)0;
	cnd_destroy(&this->queue);
}

static inline int
queue_check (Queue* this)
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

static inline int
queue_wait (Queue* this)
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

static ALWAYS inline int
queue_notify (Queue* this)
{
	++this->permits;
	return cnd_signal(&this->queue);
}

static ALWAYS inline int
queue_broadcast (Queue* this)
{
	if (this->waiting > 0) {
		this->permits += this->waiting;
		return cnd_broadcast(&this->queue);
	}
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
