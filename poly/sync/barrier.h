#ifndef POLY_BARRIER_H
#define POLY_BARRIER_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/condition.h"

////////////////////////////////////////////////////////////////////////
// Barrier interface
////////////////////////////////////////////////////////////////////////

typedef struct Barrier {
	Lock        syncronized;
	Condition   queue;
	signed      counter;    // # of threads still expected before opening
	signed      capacity;   // # of threads to wait before opening the barrier
	unsigned    phase;      // an unique token for each phase
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
		assert(this->counter > 0);\
		assert(this->counter <= this->capacity);
#else
#	define ASSERT_BARRIER_INVARIANT
#endif

static int
barrier_init (Barrier *const this, int capacity)
{
	assert(capacity > 1);

	this->counter = this->capacity = capacity;
	this->phase = 0; // overflow is wellcome

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=condition_init(&this->queue)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_BARRIER_INVARIANT

	return STATUS_SUCCESS;
}

static void
barrier_destroy (Barrier *const this)
{
	assert(this->counter == this->capacity); // empty

	condition_destroy(&this->queue);
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

	switch (this->counter) {
		case 1:
			this->counter = this->capacity;
			++this->phase;
			catch (condition_broadcast(&this->queue));
			if (last != NULL) { *last = true; }
			break;
		default: // >= 2
			--this->counter;
			unsigned const phase = this->phase;
			do {
				catch (condition_wait(&this->queue, &this->syncronized));
			} while (phase == this->phase);
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
