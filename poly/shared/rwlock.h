#ifndef RWLOCK_H
#define RWLOCK_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct RWLock {
	signed counter; // -1: writing; 0: idle; >0: # of active readers
	Lock   entry;
	Notice readers;
	Notice writers;
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

enum { RWLOCK_IDLE=0, RWLOCK_WRITING=-1 };

static inline int
rwlock_init (RWLock* this)
{
	this->counter = RWLOCK_IDLE;

	int err;
	if ((err=lock_init(&this->entry)) == STATUS_SUCCESS) {
		if ((err=notice_init(&this->readers, lock)) == STATUS_SUCCESS) {
			if ((err=notice_init(&this->writers, lock)) == STATUS_SUCCESS) {
				/*skip*/;
			} else {
				notice_destroy(&this->readers);
				lock_destroy(&this->entry);
			}
		} else {
			lock_destroy(&this->entry);
		}
	}
	return err;
}

static inline void
rwlock_destroy (RWLock* this)
{
	assert(this->counter == RWLOCK_IDLE);

	notice_destroy(&this->readers);
	notice_destroy(&this->writers);
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
static inline int
rwlock_acquire (RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	if (this->counter != RWLOCK_IDLE) {
		int err = notice_check(&this->writers);
		CHECK_RWLOCK_MONITOR (err)
		assert(this->counter == RWLOCK_IDLE);
	}
	this->counter = RWLOCK_WRITING;

	LEAVE_RWLOCK_MONITOR
}

static inline int
rwlock_release (RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	this->counter = RWLOCK_IDLE;
	if (!notice_empty(&this->writers)) {
		int err = notice_notify(&this->writers);
		CHECK_RWLOCK_MONITOR (err)
	} else if (!notice_empty(&this->readers)) {
		int err = notice_broadcast(&this->readers);
		CHECK_RWLOCK_MONITOR (err)
	}

	LEAVE_RWLOCK_MONITOR
}

//
// Readers
//
static inline int
rwlock_enter (RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	while (this->counter == RWLOCK_WRITING) {
		int err = notice_wait(&this->readers);
		CHECK_RWLOCK_MONITOR (err)
	}
	++this->counter;

	LEAVE_RWLOCK_MONITOR
}

static inline int
rwlock_leave (RWLock* this)
{
	ENTER_RWLOCK_MONITOR

	if (--this->counter == RWLOCK_IDLE) {
		if (!notice_empty(&this->writers)) {
			int err = notice_notify(&this->writers);
			CHECK_RWLOCK_MONITOR (err)
		}
	}

	LEAVE_RWLOCK_MONITOR
}

#undef ENTER_RWLOCK_MONITOR
#undef LEAVE_RWLOCK_MONITOR
#undef CHECK_RWLOCK_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
