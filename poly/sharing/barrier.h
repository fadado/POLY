#ifndef BARRIER_H
#define BARRIER_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Barrier {
	unsigned capacity; // # of threads to wait before opening the barrier
	unsigned places;   // # of threads still expected before opening
	Lock     monitor;
	Notice   move_on;
} Barrier;

static int  barrier_init(Barrier *const this);
static void barrier_destroy(Barrier *const this);
static int  barrier_wait(Barrier *const this);

enum { BARRIER_FULL = -1 };

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
//
static ALWAYS inline int
_barrier_empty (Barrier const*const this)
{
	return this->places == this->capacity;
}
*/

static int
barrier_init (Barrier *const this, int capacity)
{
	assert(capacity > 1);

	this->places = this->capacity = capacity;

	int err;
	if ((err=lock_init(&this->monitor)) == STATUS_SUCCESS) {
		if ((err=notice_init(&this->move_on, &this->monitor)) == STATUS_SUCCESS) {
			/*skip*/;
		} else {
			lock_destroy(&this->monitor);
		}
	}
	return err;
}

static void
barrier_destroy (Barrier *const this)
{
	assert(this->places == 0);

	notice_destroy(&this->move_on);
	lock_destroy(&this->monitor);
}

//
//Monitor helpers
//
#define ENTER_BARRIER_MONITOR\
	int err_;\
	if ((err_=lock_acquire(&this->monitor))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_BARRIER_MONITOR\
	if ((err_=lock_release(&this->monitor))!=STATUS_SUCCESS)\
	return err_;\

#define CHECK_BARRIER_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lock_release(&this->monitor);\
		return (E);\
	}

static inline int
barrier_wait (Barrier *const this)
{
	int status = STATUS_SUCCESS;
	ENTER_BARRIER_MONITOR

	if (--this->places == 0) {
		this->places = this->capacity;
		status  = BARRIER_FULL;
		const int err = notice_broadcast(&this->move_on);
		CHECK_BARRIER_MONITOR (err)
	} else {
		int const err = notice_wait(&this->move_on);
		CHECK_BARRIER_MONITOR (err)
	}

	LEAVE_BARRIER_MONITOR
	return status;
}

#undef ENTER_BARRIER_MONITOR
#undef LEAVE_BARRIER_MONITOR
#undef CHECK_BARRIER_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
