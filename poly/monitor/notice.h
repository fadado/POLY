#ifndef NOTICE_H
#define NOTICE_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Notice {
	unsigned permits; // # of threads allowed to leave the queue
	unsigned waiting; // # of threads waiting in the queue
	mtx_t*   mutex;   // monitor lock
	cnd_t    queue;   // monitor condition
} Notice;

static inline int  notice_broadcast(Notice* this);
static inline int  notice_check(Notice* this);
static inline void notice_destroy(Notice* this);
static inline bool notice_empty(Notice* this);
static inline int  notice_init(Notice* this, union Lock lock);
static inline int  notice_notify(Notice* this);
static inline int  notice_wait(Notice* this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
static ALWAYS inline int
_notice_length (Notice* this)
{
	return this->waiting;
}
*/

static ALWAYS inline bool
notice_empty (Notice* this)
{
	return this->waiting == 0;
}

static inline int
notice_init (Notice* this, union Lock lock)
{
	this->waiting = this->permits = 0;
	this->mutex = lock.mutex;
	return cnd_init(&this->queue);
}

static inline void
notice_destroy (Notice* this)
{
	assert(this->permits == 0);
	assert(this->waiting == 0);

	this->mutex = (mtx_t*)0;
	cnd_destroy(&this->queue);
}

static inline int
notice_check (Notice* this)
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
notice_wait (Notice* this)
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
notice_notify (Notice* this)
{
	++this->permits;
	return cnd_signal(&this->queue);
}

static ALWAYS inline int
notice_broadcast (Notice* this)
{
	if (this->waiting > 0) {
		this->permits += this->waiting;
		return cnd_broadcast(&this->queue);
	}
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
