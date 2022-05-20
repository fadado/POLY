#ifndef POLY_RWLOCK_H
#define POLY_RWLOCK_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// RWLock interface
////////////////////////////////////////////////////////////////////////

typedef struct RWLock {
	Lock    syncronized;
	Notice  readers;
	Notice  writers;
	signed  counter; // -1: writing; 0: idle; >0: # of active readers
} RWLock;

static int  rwlock_acquire(RWLock *const this);
static void rwlock_destroy(RWLock *const this);
static int  rwlock_enter(RWLock *const this);
static int  rwlock_init(RWLock *const this);
static int  rwlock_leave(RWLock *const this);
static int  rwlock_release(RWLock *const this);

////////////////////////////////////////////////////////////////////////
// RWLock implementation
////////////////////////////////////////////////////////////////////////

/*  RWLock rw;
 *
 *  catch (rwlock_init(&rw));
 *  ...
 *  rwlock_destroy(&rw);
 */

enum { RWLOCK_IDLE=0, RWLOCK_WRITING=-1 };

static int
rwlock_init (RWLock *const this)
{
	this->counter = RWLOCK_IDLE;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=notice_init(&this->readers, &this->syncronized)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	if ((err=notice_init(&this->writers, &this->syncronized)) != STATUS_SUCCESS) {
		notice_destroy(&this->readers);
		lock_destroy(&this->syncronized);
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
	lock_destroy(&this->syncronized);
}

/*
 * catch (rwlock_acquire(&rw)) | catch (rwlock_enter(&rw)) | catch (rwlock_enter(&rw))
 * ...                         | ...                       | ...
 * catch (rwlock_release(&rw)) | catch (rwlock_leave(&rw)) | catch (rwlock_leave(&rw)) 
 */

static inline int
rwlock_acquire (RWLock *const this)
{
	int err;
	enter_monitor(this);

	if (this->counter != RWLOCK_IDLE) {
		catch (notice_wait(&this->writers));
		assert(this->counter == RWLOCK_IDLE);
	}
	this->counter = RWLOCK_WRITING;

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static inline int
rwlock_release (RWLock *const this)
{
	int err;
	enter_monitor(this);

	this->counter = RWLOCK_IDLE;
	if (notice_ready(&this->writers)) {
		catch (notice_signal(&this->writers));
	} else if (notice_ready(&this->readers)) {
		catch (notice_broadcast(&this->readers));
	}

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static inline int
rwlock_enter (RWLock *const this)
{
	int err;
	enter_monitor(this);

	while (this->counter == RWLOCK_WRITING) {
		catch (notice_do_wait(&this->readers));
	}
	++this->counter;

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static inline int
rwlock_leave (RWLock *const this)
{
	int err;
	enter_monitor(this);

	if (--this->counter == RWLOCK_IDLE) {
		if (notice_ready(&this->writers)) {
			catch (notice_signal(&this->writers));
		}
	}

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
