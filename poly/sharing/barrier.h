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
	signed      capacity;   // # of threads to wait before opening the barrier
	signed      count;      // # of threads still expected before opening
	unsigned    episode;    // an unique token for each episode
} Barrier;

static int  barrier_init(Barrier *const this, int capacity);
static void barrier_destroy(Barrier *const this);
static int  barrier_wait(Barrier *const this, bool last[static 1]);

////////////////////////////////////////////////////////////////////////
// Barrier implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#   define ASSERT_BARRIER_INVARIANT  \
        assert(this->capacity >= 0); \
        assert(0 < this->count       \
                && this->count <= this->capacity);
#else
#   define ASSERT_BARRIER_INVARIANT
#endif

static int
barrier_init (Barrier *const this, int capacity)
{
	assert(capacity > 1);

	this->count = this->capacity = capacity;
	this->episode = 0; // 0,1,2...MAX,0,1,... (overflow is welcome)

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
	assert(this->count == this->capacity); // empty

	condition_destroy(&this->queue);
	lock_destroy(&this->syncronized);
}

/*
 * catch (barrier_wait(&b,0)); | catch (barrier_wait(&b,0)); | N threads 
 *
 * How to detect end of episode:
 *
 * bool last = false;
 * catch (barrier_wait(&b, &last));
 * if (last) ...
 *
 */
static int
barrier_wait (Barrier *const this, bool last[static 1])
{
	MONITOR_ENTRY

	switch (this->count) {
		default: // >= 2
			--this->count;
			// wait until this episode is complete
			unsigned const e = this->episode;
			do {
				catch (condition_wait(&this->queue, &this->syncronized));
			} while (e == this->episode);
			break;
		case 1:
			this->count = this->capacity;
			// signal episode completion
			++this->episode;
			catch (condition_broadcast(&this->queue));
			last[0] = true;
			break;
		case 0:
			assert(internal_error);
			break;
	}
	ASSERT_BARRIER_INVARIANT

	ENTRY_END
}

#undef ASSERT_BARRIER_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
