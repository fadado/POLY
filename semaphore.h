/*
 * Semaphore
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Type
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	int   counter; // <0: abs(#) of blocked threads; 0: idle; >0: available resources
	Lock  entry;
	Event queue;
} Semaphore;

static inline void semaphore_destroy(Semaphore* this);
static inline int  semaphore_init(Semaphore* this, int count);
static inline int  semaphore_P(Semaphore* this);
static inline int  semaphore_V(Semaphore* this);

#define            semaphore_down(s) semaphore_P(s)
#define            semaphore_wait(s) semaphore_P(s)
#define            semaphore_acquire(s) semaphore_P(s)

#define            semaphore_up(s) semaphore_V(s)
#define            semaphore_post(s) semaphore_V(s)
#define            semaphore_signal(s) semaphore_V(s)
#define            semaphore_release(s) semaphore_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
// Number of available resources
static ALWAYS inline int _semaphore_value(Semaphore* this)
{
	return (this->counter > 0) ? this->counter : 0;
}

// Number of blocked threads in the queue
static ALWAYS inline int _semaphore_length(Semaphore* this)
{
	return (this->counter < 0) ? -this->counter : 0;
}

// Idle state ("red" semaphore)? value==0 and length==0
static ALWAYS inline bool _semaphore_idle(Semaphore* this)
{
	return this->counter == 0;
}
*/

static inline int semaphore_init(Semaphore* this, int count)
{
	assert(count >= 0);

	this->counter = count;

	int err;
	if ((err=lock_init(&this->entry)) == STATUS_SUCCESS) {
		if ((err=event_init(&this->queue, &this->entry)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			lock_destroy(&this->entry);
		}
	}
	return err;
}

static inline void semaphore_destroy(Semaphore* this)
{
	assert(this->counter == 0 );

	event_destroy(&this->queue);
	lock_destroy(&this->entry);
}

//
//Monitor helpers
//
#define ENTER_SEMAPHORE_MONITOR\
	int err_;\
	if ((err_=lock_acquire(&this->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_SEMAPHORE_MONITOR\
	if ((err_=lock_release(&this->entry))!=STATUS_SUCCESS)\
	return err_;\
	else return STATUS_SUCCESS;

#define CHECK_SEMAPHORE_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lock_release(&this->entry);\
		return (E);\
	}

/*
Decrements the value of semaphore variable by 1. If the new value of 
this->counter is negative, the process executing wait is blocked (i.e.,
added to the semaphore's queue). Otherwise, the process continues execution,
having used a unit of the resource.
*/
static inline int semaphore_P(Semaphore* this)
{
	ENTER_SEMAPHORE_MONITOR

	--this->counter;
	register int length = this->counter < 0 ? -this->counter : 0;
	if (length > 0) {
		int err = event_stay(&this->queue);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_SEMAPHORE_MONITOR
}

/*
Increments the value of semaphore variable by 1. After the increment, if the
value is negative or zero (meaning there are processes waiting for a
resource), it transfers a blocked process from the semaphore's waiting queue
to the ready queue.
*/
static inline int semaphore_V(Semaphore* this)
{
	ENTER_SEMAPHORE_MONITOR

	int length = this->counter < 0 ? -this->counter : 0;
	++this->counter;
	if (length > 0) {
		int err = event_notify(&this->queue);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_SEMAPHORE_MONITOR
}

#undef ENTER_SEMAPHORE_MONITOR
#undef LEAVE_SEMAPHORE_MONITOR
#undef CHECK_SEMAPHORE_MONITOR

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
