#ifndef NOTICE_H
#define NOTICE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "lock.h"

/*
 * An enhanced replacement for C11 type `cnd_t`.
 */

////////////////////////////////////////////////////////////////////////
// Notice interface
////////////////////////////////////////////////////////////////////////

typedef struct Notice {
	mtx_t*  mutex;
	cnd_t   queue;
	signed  permits; // # of threads allowed to leave the queue
	signed  waiting; // # of threads waiting in the queue
} Notice;

static int  notice_broadcast(Notice *const this);
static void notice_destroy(Notice *const this);
static int  notice_do_wait(Notice *const this);
static int  notice_init(Notice *const this, union Lock lock);
static int  notice_notify(Notice *const this);
static bool notice_ready(Notice const*const this);
static int  notice_wait(Notice *const this);

////////////////////////////////////////////////////////////////////////
// Notice implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_NOTICE_INVARIANT\
		assert(this->permits >= 0);\
		assert(this->waiting >= 0);\
		assert(this->mutex != NULL);
#else
#	define ASSERT_NOTICE_INVARIANT
#endif

static inline int
notice_init (Notice *const this, union Lock lock)
{
	this->waiting = this->permits = 0;
	this->mutex = lock.mutex;

	const int err = cnd_init(&this->queue);
	ASSERT_NOTICE_INVARIANT

	return err;
}

static inline void
notice_destroy (Notice *const this)
{
	assert(this->permits == 0);
	assert(this->waiting == 0);

	this->mutex = NULL;
	cnd_destroy(&this->queue);
}

static ALWAYS inline bool
notice_ready (Notice const*const this)
{
	return this->waiting != 0; // thread safe?
}

static inline int
notice_await (Notice *const this, bool(predicate)(void))
{
	// until predicate()
	while (!predicate()) {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	}
	--this->permits; // TODO: permits < 0 !!!!!!!!!!!!!!!!!!!!
	//ASSERT_NOTICE_INVARIANT
	//if (this->permits<0) warn("permits: %d", this->permits);

	return STATUS_SUCCESS;
}

static inline int
notice_wait (Notice *const this)
{
	// until permits > 0
	while (this->permits == 0) {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	}
	--this->permits;
	ASSERT_NOTICE_INVARIANT

	return STATUS_SUCCESS;
}

static inline int
notice_do_wait (Notice *const this)
{
	do {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err != STATUS_SUCCESS) return err;
	// until permits > 0
	} while (this->permits == 0);
	--this->permits;
	ASSERT_NOTICE_INVARIANT

	return STATUS_SUCCESS;
}

static ALWAYS inline int
notice_notify (Notice *const this)
{
	++this->permits;
	const int err = cnd_signal(&this->queue);
	ASSERT_NOTICE_INVARIANT

	return err;
}

static ALWAYS inline int
notice_broadcast (Notice *const this)
{
	if (this->waiting == 0) {
		return STATUS_SUCCESS;
	}
	this->permits += this->waiting;
	const int err = cnd_broadcast(&this->queue);
	ASSERT_NOTICE_INVARIANT

	return err;
}

#undef ASSERT_NOTICE_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
