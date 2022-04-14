#ifndef RWLOCK_H
#define RWLOCK_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct RWLock {
	Lock   monitor;
	Notice readers;
	Notice writers;
	signed counter; // -1: writing; 0: idle; >0: # of active readers
} RWLock;

static int  rwlock_acquire(RWLock *const this);
static void rwlock_destroy(RWLock *const this);
static int  rwlock_enter(RWLock *const this);
static int  rwlock_init(RWLock *const this);
static int  rwlock_leave(RWLock *const this);
static int  rwlock_release(RWLock *const this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

enum { RWLOCK_IDLE=0, RWLOCK_WRITING=-1 };

static int
rwlock_init (RWLock *const this)
{
	this->counter = RWLOCK_IDLE;

	int err;
	if ((err=(lock_init(&this->monitor))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=notice_init(&this->readers, &this->monitor)) != STATUS_SUCCESS) {
		lock_destroy(&this->monitor);
		return err;
	}
	if ((err=notice_init(&this->writers, &this->monitor)) != STATUS_SUCCESS) {
		notice_destroy(&this->readers);
		lock_destroy(&this->monitor);
		return err;
	}

	return STATUS_SUCCESS;
}

static void
rwlock_destroy (RWLock *const this)
{
	assert(this->counter == RWLOCK_IDLE);

	notice_destroy(&this->readers);
	notice_destroy(&this->writers);
	lock_destroy(&this->monitor);
}

//
//Monitor helpers
//
#define ENTER_MONITOR\
	if ((err=lock_acquire(&this->monitor))!=STATUS_SUCCESS){\
		return err;\
	}

#define LEAVE_MONITOR\
	if ((err=lock_release(&this->monitor))!=STATUS_SUCCESS){\
		return err;\
	}

//
// Writer 
//
static inline int
rwlock_acquire (RWLock *const this)
{
	int err;
	ENTER_MONITOR

	if (this->counter != RWLOCK_IDLE) {
		catch (notice_enquire(&this->writers));
		assert(this->counter == RWLOCK_IDLE);
	}
	this->counter = RWLOCK_WRITING;

	LEAVE_MONITOR
	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

static inline int
rwlock_release (RWLock *const this)
{
	int err;
	ENTER_MONITOR

	this->counter = RWLOCK_IDLE;
	if (!notice_empty(&this->writers)) {
		catch (notice_notify(&this->writers));
	} else if (!notice_empty(&this->readers)) {
		catch (notice_broadcast(&this->readers));
	}

	LEAVE_MONITOR
	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

//
// Readers
//
static inline int
rwlock_enter (RWLock *const this)
{
	int err;
	ENTER_MONITOR

	while (this->counter == RWLOCK_WRITING) {
		catch (notice_wait(&this->readers));
	}
	++this->counter;

	LEAVE_MONITOR
	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

static inline int
rwlock_leave (RWLock *const this)
{
	int err;
	ENTER_MONITOR

	if (--this->counter == RWLOCK_IDLE) {
		if (!notice_empty(&this->writers)) {
			catch (notice_notify(&this->writers));
		}
	}

	LEAVE_MONITOR
	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

#undef ENTER_MONITOR
#undef LEAVE_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
