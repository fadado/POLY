#ifndef BARRIER_H
#define BARRIER_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Barrier interface
////////////////////////////////////////////////////////////////////////

typedef struct Barrier {
	Lock    syncronized;
	Notice  move_on;
	signed  capacity; // # of threads to wait before opening the barrier
	signed  places;   // # of threads still expected before opening
} Barrier;

static int  barrier_init(Barrier *const this, int capacity);
static void barrier_destroy(Barrier *const this);
static int  barrier_wait(Barrier *const this);

enum { BARRIER_FULL = -1 };

////////////////////////////////////////////////////////////////////////
// Barrier implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_BARRIER_INVARIANT\
		assert(this->capacity >= 0);\
		assert(this->places >= 0);\
		assert(this->places <= this->capacity);
#else
#	define ASSERT_BARRIER_INVARIANT
#endif

static ALWAYS inline int
_barrier_empty (Barrier const*const this)
{
	return this->places == this->capacity;
}

/*  Barrier b;
 *
 *  int N = # of thread to wait before opening the barrier
 *  catch (barrier_init(&b, N));
 *  ...
 *  barrier_destroy(&b);
 */

static int
barrier_init (Barrier *const this, int capacity)
{
	assert(capacity > 1);

	this->places = this->capacity = capacity;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=notice_init(&this->move_on, &this->syncronized)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_BARRIER_INVARIANT

	return STATUS_SUCCESS;
}

static void
barrier_destroy (Barrier *const this)
{
	assert(_barrier_empty(this));

	notice_destroy(&this->move_on);
	lock_destroy(&this->syncronized);
}

/*
 * catch (barrier_wait(&b)) | catch (barrier_wait(&b)) | N threads 
 *
 * How to detect end of cycle:
 *
 * switch (err = barrier_wait(&b)) {
 *     default:             goto onerror
 *     case BARRIER_FULL:   cycle completed
 *     case STATUS_SUCCESS: one place assigned
 * }
 *
 */
static inline int
barrier_wait (Barrier *const this)
{
	int status = STATUS_SUCCESS;

	int err;
	enter_monitor(this);

	if (--this->places != 0) {
		catch (notice_wait(&this->move_on));
	} else {
		status = BARRIER_FULL;
		this->places = this->capacity;
		catch (notice_broadcast(&this->move_on));
	}
	ASSERT_BARRIER_INVARIANT

	leave_monitor(this);
	return status;
onerror:
	break_monitor(this);
	return err;
}

#undef ASSERT_BARRIER_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
