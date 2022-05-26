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
	signed      value;      // # of threads still expected before opening
	signed      capacity;   // # of threads to wait before opening the barrier
	unsigned    cycle;      // an unique token for each cycle
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
		assert(0 < this->value && this->value <= this->capacity);
#else
#	define ASSERT_BARRIER_INVARIANT
#endif

static int
barrier_init (Barrier *const this, int capacity)
{
	assert(capacity > 1);

	this->value = this->capacity = capacity;
	this->cycle = 0; // 0,1,2...MAX,0,1,... (overflow is welcome)

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
	assert(this->value == this->capacity); // empty

	condition_destroy(&this->queue);
	lock_destroy(&this->syncronized);
}

/*
 * catch (barrier_wait(&b,0)) | catch (barrier_wait(&b,0)) | N threads 
 *
 * How to detect end of cycle:
 *
 * bool last = false;
 * catch (barrier_wait(&b, &last));
 * if (last) ...
 *
 */
static int
barrier_wait (Barrier *const this, bool* last)
{
	int err;
	enter_monitor(this);

	switch (this->value) {
		default: // >= 2
			--this->value;
			unsigned const cycle = this->cycle;
			do {
				catch (condition_wait(&this->queue, &this->syncronized));
			} while (cycle == this->cycle);
			break;
		case 1:
			this->value = this->capacity;
			++this->cycle;
			catch (condition_broadcast(&this->queue));
			if (last != NULL) { *last = true; }
			break;
		case 0:
			assert(internal_error);
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
