#ifndef POLY_NOTICE_H
#define POLY_NOTICE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "lock.h"
#include "condition.h"

/*
 * An enhanced replacement for C11 type `cnd_t`.
 */

////////////////////////////////////////////////////////////////////////
// Notice interface
////////////////////////////////////////////////////////////////////////

typedef struct Notice {
	union Lock  lock;
	Condition   queue;
	signed      permits; // # of threads allowed to leave the queue
	signed      waiting; // # of threads waiting in the queue
} Notice;

static int  notice_broadcast(Notice *const this);
static void notice_destroy(Notice *const this);
static int  notice_do_wait(Notice *const this);
static int  notice_init(Notice *const this, union Lock lock);
static int  notice_signal(Notice *const this);
static bool notice_ready(Notice const*const this);
static int  notice_wait(Notice *const this);

////////////////////////////////////////////////////////////////////////
// Notice implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_NOTICE_INVARIANT  \
		assert(this->waiting >= 0); \
		assert(this->permits >= 0); \
		assert(this->lock.mutex != NULL);
#else
#	define ASSERT_NOTICE_INVARIANT
#endif

static inline int
notice_init (Notice *const this, union Lock lock)
{
	this->waiting = this->permits = 0;
	this->lock = lock;
	ASSERT_NOTICE_INVARIANT

	return condition_init(&this->queue);
}

static inline void
notice_destroy (Notice *const this)
{
	assert(this->permits == 0);
	assert(this->waiting == 0);

	this->lock.mutex = NULL;
	condition_destroy(&this->queue);
}

static ALWAYS inline bool
notice_ready (Notice const*const this)
{
	return this->waiting != 0; // thread safe?
}

////////////////////////////////////////////////////////////////////////

#define _notice_enqueue(this)\
	++this->waiting;\
	const int err = condition_wait(&this->queue, this->lock);\
	--this->waiting;\
	if (err == STATUS_SUCCESS) continue; else return err

static inline int
notice_wait (Notice *const this)
{
	while (this->permits == 0) {
		_notice_enqueue(this);
	}
	--this->permits;
	ASSERT_NOTICE_INVARIANT

	return STATUS_SUCCESS;
}

static inline int
notice_do_wait (Notice *const this)
{
	do {
		_notice_enqueue(this);
	} while (this->permits == 0);
	--this->permits;
	ASSERT_NOTICE_INVARIANT

	return STATUS_SUCCESS;
}

#undef _notice_enqueue

////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
notice_signal (Notice *const this)
{
	++this->permits;
	ASSERT_NOTICE_INVARIANT

	return condition_signal(&this->queue);
}

static ALWAYS inline int
notice_broadcast (Notice *const this)
{
	this->permits += this->waiting;
	ASSERT_NOTICE_INVARIANT

	return condition_broadcast(&this->queue);
}

#undef ASSERT_NOTICE_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
