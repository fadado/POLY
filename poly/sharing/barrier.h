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
	Lock     entry;
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

static inline int
barrier_init (Barrier *const this, int capacity)
{
	assert(capacity > 1);

	this->places = this->capacity = capacity;

	int err;
	if ((err=lock_init(&this->entry)) == STATUS_SUCCESS) {
		if ((err=notice_init(&this->move_on, &this->entry)) == STATUS_SUCCESS) {
			/*skip*/;
		} else {
			lock_destroy(&this->entry);
		}
	}
	return err;
}

static inline void
barrier_destroy (Barrier *const this)
{
	assert(this->places == 0);

	notice_destroy(&this->move_on);
	lock_destroy(&this->entry);
}

//
//Monitor helpers
//
#define ENTER_BARRIER_MONITOR\
	int err_;\
	if ((err_=lock_acquire(&this->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_BARRIER_MONITOR\
	if ((err_=lock_release(&this->entry))!=STATUS_SUCCESS)\
	return err_;\

#define CHECK_BARRIER_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lock_release(&this->entry);\
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
