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
	signed capacity; // # of threads to wait before opening the barrier
	signed places;   // # of threads still expected before opening
	Lock   monitor;
	Notice move_on;
} Barrier;

static int  barrier_init(Barrier *const this);
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
	if ((err=lock_init(&this->monitor)) == STATUS_SUCCESS) {
		if ((err=notice_init(&this->move_on, &this->monitor)) == STATUS_SUCCESS) {
			/*skip*/;
		} else {
			lock_destroy(&this->monitor);
		}
	}
	ASSERT_BARRIER_INVARIANT

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
	if ((err=lock_acquire(&this->monitor))!=STATUS_SUCCESS)\
	return err;

#define LEAVE_BARRIER_MONITOR\
	if ((err=lock_release(&this->monitor))!=STATUS_SUCCESS)\
	return err;\

#define CHECK_BARRIER_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lock_release(&this->monitor);\
		return (E);\
	}

// Error management strategy for this module
#define catch(X)\
	if ((err=(X)) != STATUS_SUCCESS)\
		goto onerror

static inline int
barrier_wait (Barrier *const this)
{
	int status = STATUS_SUCCESS;

	int err;
	ENTER_BARRIER_MONITOR

	if (--this->places == 0) {
		this->places = this->capacity;
		status = BARRIER_FULL;
		catch (notice_broadcast(&this->move_on));
	} else {
		catch (notice_wait(&this->move_on));
	}
	ASSERT_BARRIER_INVARIANT

	LEAVE_BARRIER_MONITOR
	return status;
onerror:
	lock_release(&this->monitor);
	return err;
}

#undef catch
#undef ASSERT_BARRIER_INVARIANT
#undef ENTER_BARRIER_MONITOR
#undef LEAVE_BARRIER_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
