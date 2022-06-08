#ifndef POLY_SEMAPHORE_H
#define POLY_SEMAPHORE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/condition.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	Lock        syncronized;
	Condition   queue;
	signed      value;  // available resources
} Semaphore;

static void semaphore_destroy(Semaphore *const this);
static int  semaphore_init(Semaphore *const this, unsigned count);
static int  semaphore_P(Semaphore *const this);
static int  semaphore_V(Semaphore *const this);

#define     semaphore_acquire(s) semaphore_P(s)
#define     semaphore_release(s) semaphore_V(s)

#define     semaphore_wait(s) semaphore_P(s)
#define     semaphore_signal(s) semaphore_V(s)

#define     semaphore_down(s) semaphore_P(s)
#define     semaphore_up(s) semaphore_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#   define ASSERT_SEMAPHORE_INVARIANT \
        assert(this->value >= 0);
#else
#   define ASSERT_SEMAPHORE_INVARIANT
#endif

static int
semaphore_init (Semaphore *const this, unsigned count)
{
	this->value = count;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=condition_init(&this->queue)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_SEMAPHORE_INVARIANT

	return STATUS_SUCCESS;
}

static void
semaphore_destroy (Semaphore *const this)
{
	assert(this->value == 0 ); // ???

	condition_destroy(&this->queue);
	lock_destroy(&this->syncronized);
}

/*
 * catch (semaphore_init(&mutex, 1))
 * catch (semaphore_acquire(&mutex))
 * ...
 * catch (semaphore_release(&mutex))
 *
 * catch (semaphore_init(&event, 0))
 * catch (semaphore_wait(&event)) | catch (semaphore_signal(&event))
 *
 * catch (semaphore_init(&allocator, N))
 */

static int
semaphore_P (Semaphore *const this)
{
	MONITOR_ENTRY

	while (this->value == 0) {
		catch (condition_wait(&this->queue, &this->syncronized))
	}
	--this->value;
	ASSERT_SEMAPHORE_INVARIANT

	ENTRY_END
}

static int
semaphore_V (Semaphore *const this)
{
	MONITOR_ENTRY

	++this->value;
	catch (condition_signal(&this->queue));
	ASSERT_SEMAPHORE_INVARIANT

	ENTRY_END
}

#undef ASSERT_SEMAPHORE_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
