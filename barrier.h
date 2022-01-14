/*
 * Barrier
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef BARRIER_H
#define BARRIER_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Types
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Barrier {
	int   capacity; // # of threads to wait before opening the barrier
	int   places;   // # of threads still expected before opening
	Lock  entry;    // monitor lock
	Event move_on;  // monitor condition
} Barrier;

static inline int  brr_init(Barrier* this);
static inline void brr_destroy(Barrier* this);
static inline int  brr_wait(Barrier* this);

enum { BARRIER_FULL = -1 };

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
//
static ALWAYS inline int _brr_empty(Barrier* this)
{
	return this->places == this->capacity;
}
*/

static inline int brr_init(Barrier* this, int capacity)
{
	assert(capacity > 1);

	this->places = this->capacity = capacity;

	int err;
	if ((err=lck_init(&this->entry)) == STATUS_SUCCESS) {
		if ((err=evt_init(&this->move_on, &this->entry)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			lck_destroy(&this->entry);
		}
	}
	return err;
}

static inline void brr_destroy(Barrier* this)
{
	assert(this->places == 0);

	evt_destroy(&this->move_on);
	lck_destroy(&this->entry);
}

//
//Monitor helpers
//
#define ENTER_BARRIER_MONITOR\
	int err_;\
	if ((err_=lck_acquire(&this->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_BARRIER_MONITOR\
	if ((err_=lck_release(&this->entry))!=STATUS_SUCCESS)\
	return err_;\

#define CHECK_BARRIER_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lck_release(&this->entry);\
		return (E);\
	}

static inline int brr_wait(Event* this)
{
	int status = STATUS_SUCCESS;
	ENTER_BARRIER_MONITOR

	if (--this->places == 0) {
		this->places = this->capacity;
		status  = BARRIER_FULL;
		int err = evt_broadcast(&this->move_on);
		CHECK_BARRIER_MONITOR (err)
	} else {
		int err = evt_wait(&this->move_on);
		CHECK_BARRIER_MONITOR (err)
	}

	LEAVE_BARRIER_MONITOR
	return status;
}

#undef ENTER_BARRIER_MONITOR
#undef LEAVE_BARRIER_MONITOR
#undef CHECK_BARRIER_MONITOR

#endif // BARRIER_H

// vim:ai:sw=4:ts=4:syntax=cpp
