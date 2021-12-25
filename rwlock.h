/*
 * Multiple-Readers Single-Writer lock
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef RWLOCK_H
#define RWLOCK_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

#include "lock.h"
//#include "event.h"

////////////////////////////////////////////////////////////////////////
// Type
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct RWLock {
	int   counter;
	Lock  entry;
	cnd_t queue;
} RWLock;

static inline int  rwl_init(RWLock* self);
static inline void rwl_destroy(RWLock* self);
static inline int  rwl_acquire(RWLock* self);
static inline int  rwl_release(RWLock* self);
static inline int  rwl_enter(RWLock* self);
static inline int  rwl_leave(RWLock* self);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int rwl_init(RWLock* self)
{
	self->counter = 0;

	int err;
	if ((err=lck_init(&self->entry)) == STATUS_SUCCESS) {
		if ((err=cnd_init(&self->queue)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			lck_destroy(&self->entry);
		}
	}
	return err;
}

static inline void rwl_destroy(RWLock* self)
{
	assert(self->counter == 0 ); // idle state

	lck_destroy(&self->entry);
	cnd_destroy(&self->queue);
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

	while (self->counter != 0) {
		int err = cnd_wait(&self->queue, &self->entry.mutex);
		CHECK_RWLOCK_MONITOR (err)
	}
	self->counter = -1;

	LEAVE_RWLOCK_MONITOR
}

static inline int rwl_release(RWLock* self)
{
	ENTER_RWLOCK_MONITOR

	self->counter = 0;
	cnd_broadcast(&self->queue);

	LEAVE_RWLOCK_MONITOR
}

//
// Readers
//
static inline int rwl_enter(RWLock* self)
{
	ENTER_RWLOCK_MONITOR

	while (self->counter < 0) {
		int err = cnd_wait(&self->queue, &self->entry.mutex);
		CHECK_RWLOCK_MONITOR (err)
	}
	++self->counter;

	LEAVE_RWLOCK_MONITOR
}

static inline int rwl_leave(RWLock* self)
{
	ENTER_RWLOCK_MONITOR

	--self->counter;
	if (self->counter == 0) {
		cnd_broadcast(&self->queue);
	}

	LEAVE_RWLOCK_MONITOR
}

#undef ENTER_RWLOCK_MONITOR
#undef LEAVE_RWLOCK_MONITOR
#undef CHECK_RWLOCK_MONITOR

#endif // RWLOCK_H

// vim:ai:sw=4:ts=4:syntax=cpp
