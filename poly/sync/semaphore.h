#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	Lock   syncronized;
	Notice queue;
	signed counter; // <0: abs(#) of blocked threads;
                    //  0: idle;
                    // >0: available resources
} Semaphore;

static void semaphore_destroy(Semaphore *const this);
static int  semaphore_init(Semaphore *const this, unsigned count);
static int  semaphore_P(Semaphore *const this);
static int  semaphore_V(Semaphore *const this);

#define     semaphore_down(s) semaphore_P(s)
#define     semaphore_wait(s) semaphore_P(s)
#define     semaphore_acquire(s) semaphore_P(s)

#define     semaphore_up(s) semaphore_V(s)
#define     semaphore_post(s) semaphore_V(s)
#define     semaphore_signal(s) semaphore_V(s)
#define     semaphore_release(s) semaphore_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
// Number of available resources
static ALWAYS inline int
_semaphore_value (Semaphore *const this)
{
	return (this->counter > 0) ? this->counter : 0;
}

// Number of blocked threads in the queue
static ALWAYS inline int
_semaphore_waiting (Semaphore *const this)
{
	return (this->counter < 0) ? -this->counter : 0;
}

// Idle state ("red" semaphore)? value==0 and waiting==0
static ALWAYS inline bool
_semaphore_idle (Semaphore *const this)
{
	return this->counter == 0;
}
*/

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
	assert(this->counter == 0 );

	notice_destroy(&this->queue);
	lock_destroy(&this->syncronized);
}

/*
Decrements the value of semaphore variable by 1. If the new value of 
this->counter is negative, the process executing wait is blocked (i.e.,
added to the semaphore's queue). Otherwise, the process continues execution,
having used a unit of the resource.
*/
static inline int
semaphore_P (Semaphore *const this)
{
	int err;
	enter_monitor(this);

	--this->counter;
	unsigned const waiting = (this->counter < 0) ? -this->counter : 0;
	if (waiting > 0) {
		catch (notice_wait(&this->queue));
	}

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

/*
Increments the value of semaphore variable by 1. After the increment, if the
value is negative or zero (meaning there are processes waiting for a
resource), it transfers a blocked process from the semaphore's waiting queue
to the ready queue.
*/
static inline int
semaphore_V (Semaphore *const this)
{
	int err;
	enter_monitor(this);

	unsigned const waiting = (this->counter < 0) ? -this->counter : 0;
	++this->counter;
	if (waiting > 0) {
		catch (notice_notify(&this->queue));
	}

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
