#ifndef POLY_BARRIER_H
#define POLY_BARRIER_H

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
static int  barrier_wait(Barrier *const this, bool* last);

////////////////////////////////////////////////////////////////////////
// Barrier implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_BARRIER_INVARIANT\
		assert(this->capacity >= 0);\
		assert(this->places > 0);\
		assert(this->places <= this->capacity);
#else
#	define ASSERT_BARRIER_INVARIANT
#endif

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
	assert(this->places == this->capacity); // empty

	notice_destroy(&this->move_on);
	lock_destroy(&this->syncronized);
}

/*
 * catch (barrier_wait(&b,0)) | catch (barrier_wait(&b,0)) | N threads 
 *
 * How to detect end of phase:
 *
 * bool last = false;
 * catch (barrier_wait(&b, &last));
 * if (last) ...
 *
 */
static inline int
barrier_wait (Barrier *const this, bool* last)
{
	int err;
	enter_monitor(this);

	switch (this->places) {
		case 1:
			this->places = this->capacity;
			catch (notice_broadcast(&this->move_on));
			if (last != NULL) { *last = true; }
			break;
		default: // >= 2
			--this->places;
			catch (notice_do_wait(&this->move_on));
			break;
	}
	ASSERT_BARRIER_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#undef ASSERT_BARRIER_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
