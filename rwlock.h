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
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Type
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct RWLock {
	int   counter; // -1: a reader holds the lock; 0: ; >0: # of active writers
	Lock  entry;
	Event readers;
	Event writers;
} RWLock;

static inline int  rwl_acquire(RWLock* self);
static inline void rwl_destroy(RWLock* self);
static inline int  rwl_enter(RWLock* self);
static inline int  rwl_init(RWLock* self);
static inline int  rwl_leave(RWLock* self);
static inline int  rwl_release(RWLock* self);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int rwl_init(RWLock* self)
{
	self->counter = 0;

	int err;
	if ((err=lck_init(&self->entry)) == STATUS_SUCCESS) {
		if ((err=evt_init(&self->readers, &self->entry)) == STATUS_SUCCESS) {
			if ((err=evt_init(&self->writers, &self->entry)) == STATUS_SUCCESS) {
				return STATUS_SUCCESS;
			} else {
				evt_destroy(&self->readers);
				lck_destroy(&self->entry);
			}
		} else {
			lck_destroy(&self->entry);
		}
	}
	return err;
}

static inline void rwl_destroy(RWLock* self)
{
	assert(self->counter == 0 ); // idle state

	evt_destroy(&self->readers);
	evt_destroy(&self->writers);
	lck_destroy(&self->entry);
}

//
//Monitor helpers
//
#define ENTER_RWLOCK_MONITOR\
	int err_;\
	if ((err_=lck_acquire(&self->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_RWLOCK_MONITOR\
	if ((err_=lck_release(&self->entry))!=STATUS_SUCCESS)\
	return err_;\
	else return STATUS_SUCCESS;

#define CHECK_RWLOCK_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lck_release(&self->entry);\
		return (E);\
	}

//
// Writer 
//
static inline int rwl_acquire(RWLock* self)
{
	ENTER_RWLOCK_MONITOR

	if (self->counter != 0) {
		int err = evt_wait(&self->writers);
		CHECK_RWLOCK_MONITOR (err)
		// after signaling writers the counter must be zero
		assert(self->counter == 0);
	}
	self->counter = -1; // the writer holds the lock

	LEAVE_RWLOCK_MONITOR
}

static inline int rwl_release(RWLock* self)
{
	ENTER_RWLOCK_MONITOR

	self->counter = 0; // the writer unholds the lock
	if (!_evt_empty(&self->writers)) {
		int err = evt_signal(&self->writers);
		CHECK_RWLOCK_MONITOR (err)
	} else if (!_evt_empty(&self->readers)) {
		int err = evt_broadcast(&self->readers);
		CHECK_RWLOCK_MONITOR (err)
	}

	LEAVE_RWLOCK_MONITOR
}

//
// Readers
//
static inline int rwl_enter(RWLock* self)
{
	ENTER_RWLOCK_MONITOR

	// while a writer holds the lock
	while (self->counter == -1) {
		int err = evt_wait(&self->readers);
		CHECK_RWLOCK_MONITOR (err)
	}
	++self->counter;

	LEAVE_RWLOCK_MONITOR
}

static inline int rwl_leave(RWLock* self)
{
	ENTER_RWLOCK_MONITOR

	if (--self->counter == 0) { // no readers reading
		if (!_evt_empty(&self->writers)) {
			int err = evt_signal(&self->writers);
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
