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
	Lock   monitor;
	Notice move_on;
	signed capacity; // # of threads to wait before opening the barrier
	signed places;   // # of threads still expected before opening
} Barrier;

static int  barrier_init(Barrier *const this, int capacity);
static void barrier_destroy(Barrier *const this);
static int  barrier_wait(Barrier *const this);

enum { BARRIER_FULL = -1 };

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_BARRIER_INVARIANT\
		assert(this->capacity >= 0);\
		assert(this->places >= 0);\
		assert(this->places <= this->capacity);
#else
#	define ASSERT_BARRIER_INVARIANT
#endif

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
	if ((err=(lock_init(&this->monitor))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=notice_init(&this->move_on, &this->monitor)) != STATUS_SUCCESS) {
		lock_destroy(&this->monitor);
		return err;
	}
	ASSERT_BARRIER_INVARIANT

	return STATUS_SUCCESS;
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
#define ENTER_MONITOR\
	if ((err=lock_acquire(&this->monitor))!=STATUS_SUCCESS){\
		return err;\
	}

#define LEAVE_MONITOR\
	if ((err=lock_release(&this->monitor))!=STATUS_SUCCESS){\
		return err;\
	}

/* How to detect each "cycle":
 *
 * switch (barrier_wait(&b)) {
 *     case BARRIER_FULL:   cycle completed
 *     case STATUS_SUCCESS: take one place
 *     default:             error
 * }
 *
 */
static inline int
barrier_wait (Barrier *const this)
{
	int status = STATUS_SUCCESS;

	int err;
	ENTER_MONITOR

	if (--this->places == 0) {
		this->places = this->capacity;
		status = BARRIER_FULL;
		catch (notice_broadcast(&this->move_on));
	} else {
		catch (notice_wait(&this->move_on));
	}
	ASSERT_BARRIER_INVARIANT

	LEAVE_MONITOR
	return status;
onerror:
	lock_release(&this->monitor);
	return err;
}

#undef ASSERT_BARRIER_INVARIANT
#undef ENTER_MONITOR
#undef LEAVE_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
