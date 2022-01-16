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

static inline void sem_destroy(Semaphore* this);
static inline int  sem_init(Semaphore* this, int count);
static inline int  sem_P(Semaphore* this);
static inline int  sem_V(Semaphore* this);

#define            sem_down(s) sem_P(s)
#define            sem_wait(s) sem_P(s)
#define            sem_acquire(s) sem_P(s)

#define            sem_up(s) sem_V(s)
#define            sem_post(s) sem_V(s)
#define            sem_signal(s) sem_V(s)
#define            sem_release(s) sem_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
// Number of available resources
static ALWAYS inline int _sem_value(Semaphore* this)
{
	return (this->counter > 0) ? this->counter : 0;
}

// Number of blocked threads in the queue
static ALWAYS inline int _sem_length(Semaphore* this)
{
	return (this->counter < 0) ? -this->counter : 0;
}

// Idle state ("red" semaphore)? value==0 and length==0
static ALWAYS inline bool _sem_idle(Semaphore* this)
{
	return this->counter == 0;
}
*/

static inline int sem_init(Semaphore* this, int count)
{
	assert(count >= 0);

	this->counter = count;

	int err;
	if ((err=lck_init(&this->entry)) == STATUS_SUCCESS) {
		if ((err=evt_init(&this->queue, &this->entry)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			lck_destroy(&this->entry);
		}
	}
	return err;
}

static inline void sem_destroy(Semaphore* this)
{
	assert(this->counter == 0 );

	evt_destroy(&this->queue);
	lck_destroy(&this->entry);
}

//
//Monitor helpers
//
#define ENTER_SEMAPHORE_MONITOR\
	int err_;\
	if ((err_=lck_acquire(&this->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_SEMAPHORE_MONITOR\
	if ((err_=lck_release(&this->entry))!=STATUS_SUCCESS)\
	return err_;\
	else return STATUS_SUCCESS;

#define CHECK_SEMAPHORE_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lck_release(&this->entry);\
		return (E);\
	}

/*
Decrements the value of semaphore variable by 1. If the new value of 
this->counter is negative, the process executing wait is blocked (i.e.,
added to the semaphore's queue). Otherwise, the process continues execution,
having used a unit of the resource.
*/
static inline int sem_P(Semaphore* this)
{
	ENTER_SEMAPHORE_MONITOR

	--this->counter;
	int length = this->counter < 0 ? -this->counter : 0;
	if (length > 0) {
		int err = evt_stay(&this->queue);
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
static inline int sem_V(Semaphore* this)
{
	ENTER_SEMAPHORE_MONITOR

	int length = this->counter < 0 ? -this->counter : 0;
	++this->counter;
	if (length > 0) {
		int err = evt_notify(&this->queue);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_SEMAPHORE_MONITOR
}

#undef ENTER_SEMAPHORE_MONITOR
#undef LEAVE_SEMAPHORE_MONITOR
#undef CHECK_SEMAPHORE_MONITOR

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
