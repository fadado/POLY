/*
 * Multiple-Readers Single-Writer lock
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef RWLOCK_H
#define RWLOCK_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"
#include "queue.h"

////////////////////////////////////////////////////////////////////////
// Type
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct RWLock {
	int   counter; // -1: writing; 0: idle; >0: # of active readers
	Lock  entry;
	Queue readers;
	Queue writers;
} RWLock;

static inline int  rwlock_acquire(RWLock* this);
static inline void rwlock_destroy(RWLock* this);
static inline int  rwlock_enter(RWLock* this);
static inline int  rwlock_init(RWLock* this);
static inline int  rwlock_leave(RWLock* this);
static inline int  rwlock_release(RWLock* this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

enum { RWL_IDLE=0, RWL_WRITING=-1 };

static inline int rwlock_init(RWLock* this)
{
	this->counter = RWL_IDLE;

	int err;
	if ((err=lock_init(&this->entry)) == STATUS_SUCCESS) {
		if ((err=queue_init(&this->readers, &this->entry)) == STATUS_SUCCESS) {
			if ((err=queue_init(&this->writers, &this->entry)) == STATUS_SUCCESS) {
				return STATUS_SUCCESS;
			} else {
				queue_destroy(&this->readers);
				lock_destroy(&this->entry);
			}
		} else {
			lock_destroy(&this->entry);
		}
	}
	return err;
}

static inline void rwlock_destroy(RWLock* this)
{
	assert(this->counter == RWL_IDLE);

	queue_destroy(&this->readers);
	queue_destroy(&this->writers);
	lock_destroy(&this->entry);
}

//
//Monitor helpers
//
#define ENTER_RWLOCK_MONITOR\
	int err_;\
	if ((err_=lock_acquire(&this->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_RWLOCK_MONITOR\
	if ((err_=lock_release(&this->entry))!=STATUS_SUCCESS)\
	return err_;\
	else return STATUS_SUCCESS;

#define CHECK_RWLOCK_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lock_release(&this->entry);\
		return (E);\
	}

//
// Writer 
//
static inline int rwlock_acquire(RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	if (this->counter != RWL_IDLE) {
		int err = queue_check(&this->writers);
		CHECK_RWLOCK_MONITOR (err)
		assert(this->counter == RWL_IDLE);
	}
	this->counter = RWL_WRITING;

	LEAVE_RWLOCK_MONITOR
}

static inline int rwlock_release(RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	this->counter = RWL_IDLE;
	if (!queue_empty(&this->writers)) {
		int err = queue_notify(&this->writers);
		CHECK_RWLOCK_MONITOR (err)
	} else if (!queue_empty(&this->readers)) {
		int err = queue_broadcast(&this->readers);
		CHECK_RWLOCK_MONITOR (err)
	}

	LEAVE_RWLOCK_MONITOR
}

//
// Readers
//
static inline int rwlock_enter(RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	while (this->counter == RWL_WRITING) {
		int err = queue_wait(&this->readers);
		CHECK_RWLOCK_MONITOR (err)
	}
	++this->counter;

	LEAVE_RWLOCK_MONITOR
}

static inline int rwlock_leave(RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	if (--this->counter == RWL_IDLE) {
		if (!queue_empty(&this->writers)) {
			int err = queue_notify(&this->writers);
			CHECK_RWLOCK_MONITOR (err)
		}
	}

	LEAVE_RWLOCK_MONITOR
}

#undef ENTER_RWLOCK_MONITOR
#undef LEAVE_RWLOCK_MONITOR
#undef CHECK_RWLOCK_MONITOR

#endif // RWLOCK_H

// vim:ai:sw=4:ts=4:syntax=cpp
