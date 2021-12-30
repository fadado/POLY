/*
 * Barrier
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef BARRIER_H
#define BARRIER_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

#include "lock.h"
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Types
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Barrier {
	int   capacity;
	int   places;
	Lock  entry;
	Event move_on;
} Barrier;

static inline int  brr_init(Barrier* self);
static inline void brr_destroy(Barrier* self);
static inline int  brr_wait(Barrier* self);

enum { BARRIER_FULL = -1 };

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
//
static ALWAYS inline int _brr_empty(Barrier* self)
{
	return self->places == self->capacity;
}
*/

static inline int brr_init(Barrier* self, int capacity)
{
	assert(capacity > 1); // ???

	self->places = self->capacity = capacity;

	int err;
	if ((err=lck_init(&self->entry)) == STATUS_SUCCESS) {
		if ((err=evt_init(&self->move_on, &self->entry)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			lck_destroy(&self->entry);
		}
	}
	return err;
}

static inline void brr_destroy(Barrier* self)
{
	assert(self->places == 0 ); // idle state

	lck_destroy(&self->entry);
	evt_destroy(&self->move_on);
}

//
//Monitor helpers
//
#define ENTER_BARRIER_MONITOR\
	int err_;\
	if ((err_=lck_acquire(&self->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_BARRIER_MONITOR\
	if ((err_=lck_release(&self->entry))!=STATUS_SUCCESS)\
	return err_;\

#define CHECK_BARRIER_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lck_release(&self->entry);\
		return (E);\
	}

static inline int brr_wait(Event* self)
{
	int status = STATUS_SUCCESS;
	ENTER_BARRIER_MONITOR

	--self->places;
	if (self->places == 0) {
		// I'm the leader, the choosen!
		self->places = self->capacity;
		int err = evt_broadcast(&self->move_on);
		CHECK_SEMAPHORE_MONITOR (err)
		status = BARRIER_FULL; // barrier complete phase/cycle
	} else {
		int err = evt_wait(&self->move_on);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_BARRIER_MONITOR
	return status;
}

#undef ENTER_BARRIER_MONITOR
#undef LEAVE_BARRIER_MONITOR
#undef CHECK_BARRIER_MONITOR

#endif // BARRIER_H

// vim:ai:sw=4:ts=4:syntax=cpp
