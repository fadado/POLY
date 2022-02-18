#ifndef NOTICE_H
#define NOTICE_H

#ifndef POLY_H
#include "../POLY.h"
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

static int  notice_broadcast(Notice *const this);
static int  notice_check(Notice *const this);
static void notice_destroy(Notice *const this);
static int  notice_init(Notice *const this, union Lock lock);
static int  notice_notify(Notice *const this);
static bool notice_pending (Notice const*const this);
static int  notice_wait(Notice *const this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline bool
notice_pending (Notice const*const this)
{
	return this->waiting > 0;
}

static inline int
notice_init (Notice *const this, union Lock lock)
{
	this->waiting = this->permits = 0;
	this->mutex = lock.mutex;
	return cnd_init(&this->queue);
}

static inline void
notice_destroy (Notice *const this)
{
	assert(this->permits == 0);
	assert(this->waiting == 0);

	this->mutex = (mtx_t*)0;
	cnd_destroy(&this->queue);
}

static inline int
notice_check (Notice *const this)
{
	// until permits > 0
	while (this->permits == 0) {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	}
	--this->permits;
	return STATUS_SUCCESS;
}

static inline int
notice_wait (Notice *const this)
{
	do {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	// until permits > 0
	} while (this->permits == 0);
	--this->permits;
	return STATUS_SUCCESS;
}

static ALWAYS inline int
notice_notify (Notice *const this)
{
	++this->permits;
	return cnd_signal(&this->queue);
}

static ALWAYS inline int
notice_broadcast (Notice *const this)
{
	if (this->waiting > 0) {
		this->permits += this->waiting;
		return cnd_broadcast(&this->queue);
	}
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
