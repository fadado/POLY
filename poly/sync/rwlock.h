#ifndef POLY_RWLOCK_H
#define POLY_RWLOCK_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/condition.h"

////////////////////////////////////////////////////////////////////////
// RWLock interface
////////////////////////////////////////////////////////////////////////

typedef struct RWLock {
	Lock        syncronized;
	Condition   qR, qW; // queues for readers and writers
	int         nR, nW; // # of threads waiting in each queue
	signed      value;  // -1: writing; 00: idle; >0: # of active readers
} RWLock;

static int  rwlock_waitR(RWLock *const this);
static int  rwlock_waitW(RWLock *const this);
static int  rwlock_init(RWLock *const this);
static int  rwlock_signalR(RWLock *const this);
static int  rwlock_signalW(RWLock *const this);
static void rwlock_destroy(RWLock *const this);

////////////////////////////////////////////////////////////////////////
// RWLock implementation
////////////////////////////////////////////////////////////////////////

/*  RWLock rw;
 *
 *  catch (rwlock_init(&rw))
 *  ...
 *  rwlock_destroy(&rw);
 */

static int
rwlock_init (RWLock *const this)
{
	this->value = 00; // no W or R holds the lock
	this->nR = this->nW = 0;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=condition_init(&this->qR)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	if ((err=condition_init(&this->qW)) != STATUS_SUCCESS) {
		condition_destroy(&this->qR);
		lock_destroy(&this->syncronized);
		return err;
	}

	return STATUS_SUCCESS;
}

static void
rwlock_destroy (RWLock *const this)
{
	assert(this->value == 00); // no W or R holds the lock

	condition_destroy(&this->qR);
	condition_destroy(&this->qW);
	lock_destroy(&this->syncronized);
}

////////////////////////////////////////////////////////////////////////

static inline int
rwlock_waitW (RWLock *const this)
{
	MONITOR_ENTRY

	while (this->value != 00) { // wait until no W or R holds the lock
		++this->nW;
		err = condition_wait(&this->qW, &this->syncronized);
		--this->nW;
		catch (err)
	}

	this->value = -1; // writer waits the lock

	ENTRY_END
}

static inline int
rwlock_waitR (RWLock *const this)
{
	MONITOR_ENTRY

	while (this->value == -1 || this->nW > 0) { // wait until no W exists
		++this->nR;
		err = condition_wait(&this->qR, &this->syncronized);
		--this->nR;
		catch (err)
	}

	assert(this->value >= 0);
	++this->value;

	ENTRY_END
}

////////////////////////////////////////////////////////////////////////

static inline int
rwlock_signalW (RWLock *const this)
{
	MONITOR_ENTRY

	assert(this->value == -1); // writer must hold the lock

	this->value = 00; // writer signals the lock

	if (this->nW > 0) { // there are W waiting
		catch (condition_signal(&this->qW))
	} else if (this->nR > 0) { // there are R waiting
		catch (condition_broadcast(&this->qR))
	}

	ENTRY_END
}

static inline int
rwlock_signalR (RWLock *const this)
{
	MONITOR_ENTRY

	assert(this->value > 0); // some R hold the lock

	--this->value;
	if (this->value == 00) { // no W or R holds the lock
		assert(this->nR == 0);
		if (this->nW > 0) {  // there are W waiting
			catch (condition_signal(&this->qW))
		}
	}

	ENTRY_END
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
