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
		assert(this->waiting >= 0);\
		assert(this->mutex != NULL);
#	define ASSERT_NOTICE_INVARIANT_EXTRA\
		ASSERT_NOTICE_INVARIANT\
		assert(this->permits >= 0);
#else
#	define ASSERT_NOTICE_INVARIANT
#	define ASSERT_NOTICE_INVARIANT_EXTRA
#endif

static inline int
notice_init (Notice *const this, union Lock lock)
{
	this->waiting = this->permits = 0;
	this->mutex = lock.mutex;
	ASSERT_NOTICE_INVARIANT_EXTRA

	return cnd_init(&this->queue);
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

////////////////////////////////////////////////////////////////////////

static inline int
notice_wait (Notice *const this)
{
	// until permits > 0
	while (this->permits == 0) {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err == STATUS_SUCCESS) continue; else return err;
	}
	--this->permits;
	ASSERT_NOTICE_INVARIANT_EXTRA

	return STATUS_SUCCESS;
}

static inline int
notice_do_wait (Notice *const this)
{
	do {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err == STATUS_SUCCESS) continue; else return err;
	// until permits > 0
	} while (this->permits == 0);
	--this->permits;
	ASSERT_NOTICE_INVARIANT_EXTRA

	return STATUS_SUCCESS;
}

static inline int
notice_await (Notice *const this, bool(predicate)(void))
{
	// until predicate()
	while (!predicate()) {
		++this->waiting;
		const int err = cnd_wait(&this->queue, this->mutex);
		--this->waiting;
		if (err == STATUS_SUCCESS) continue; else return err;
	}
	--this->permits;
	ASSERT_NOTICE_INVARIANT

	return STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
notice_notify (Notice *const this)
{
	++this->permits;
	ASSERT_NOTICE_INVARIANT

	return cnd_signal(&this->queue);
}

static ALWAYS inline int
notice_broadcast (Notice *const this)
{
	if (this->waiting == 0) {
		return STATUS_SUCCESS;
	}
	this->permits += this->waiting;
	ASSERT_NOTICE_INVARIANT

	return cnd_broadcast(&this->queue);
}

#undef ASSERT_NOTICE_INVARIANT
#undef ASSERT_NOTICE_INVARIANT_EXTRA

#endif // vim:ai:sw=4:ts=4:syntax=cpp
