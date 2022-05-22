#ifndef POLY_SEMAPHORE_H
#define POLY_SEMAPHORE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	Lock    syncronized;
	Notice  queue;
	// counter <  0: abs(#) of blocked threads;
	// counter == 0: idle;
	// counter >  0: value (available resources)
	signed  counter;
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

static int
semaphore_init (Semaphore *const this, unsigned count)
{
	this->counter = count;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=notice_init(&this->queue, &this->syncronized)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	return STATUS_SUCCESS;
}

static void
semaphore_destroy (Semaphore *const this)
{
	assert(this->counter == 0 ); // ???

	notice_destroy(&this->queue);
	lock_destroy(&this->syncronized);
}

/*
 * catch (semaphore_init(&mutex, 1));
 * catch (semaphore_acquire(&mutex))
 * ...
 * catch (semaphore_release(&mutex))
 *
 * catch (semaphore_init(&event, 0));
 * catch (semaphore_wait(&event)) | catch (semaphore_signal(&event))
 *
 * catch (semaphore_init(&allocator, N));
 */

static int
semaphore_P (Semaphore *const this)
{
	int err;
	enter_monitor(this);

	--this->counter;
	unsigned const waiting = (this->counter < 0) ? -this->counter : 0;
	if (waiting > 0) {
		catch (notice_do_wait(&this->queue));
	}

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static int
semaphore_V (Semaphore *const this)
{
	int err;
	enter_monitor(this);

	unsigned const waiting = (this->counter < 0) ? -this->counter : 0;
	++this->counter;
	if (waiting > 0) {
		catch (notice_signal(&this->queue));
	}

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
