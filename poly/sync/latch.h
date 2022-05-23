#ifndef POLY_LATCH_H
#define POLY_LATCH_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/condition.h"

////////////////////////////////////////////////////////////////////////
// Latch interface
////////////////////////////////////////////////////////////////////////

typedef struct Latch {
	Lock        syncronized;
	Condition   queue;
	signed      value;   // # of threads still expected before opening
} Latch;

static int  latch_init(Latch *const this, int capacity);
static void latch_destroy(Latch *const this);
static int  latch_wait(Latch *const this);

////////////////////////////////////////////////////////////////////////
// Latch implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_LATCH_INVARIANT\
		assert(this->value >= 0);
#else
#	define ASSERT_LATCH_INVARIANT
#endif

static int
latch_init (Latch *const this, int capacity)
{
	assert(capacity >= 2);

	this->value = capacity;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=condition_init(&this->queue)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_LATCH_INVARIANT

	return STATUS_SUCCESS;
}

static void
latch_destroy (Latch *const this)
{
	condition_destroy(&this->queue);
	lock_destroy(&this->syncronized);
}

/*
 * catch (latch_wait(&b)) | catch (latch_wait(&b)) | N threads 
 */
static int
latch_wait (Latch *const this)
{
	int err;
	enter_monitor(this);

	switch (this->value) {
		case 0: // forever open
			break;
		case 1:
			this->value = 0;
			catch (condition_broadcast(&this->queue));
			break;
		default: // >= 2
			--this->value;
			do {
				catch (condition_wait(&this->queue, &this->syncronized));
			} while (this->value > 0);
			break;
	}
	ASSERT_LATCH_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#undef ASSERT_LATCH_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
