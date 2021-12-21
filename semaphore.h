/*
 * Semaphores
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

#include "lock.h"
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Types
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	int   counter;
	Lock  lock;
	Event one_can_wakeup;
} Semaphore;

static inline int  sem_init(Semaphore* self, int count);
static inline void sem_destroy(Semaphore* self);
static inline int  sem_P(Semaphore* self);
static inline int  sem_V(Semaphore* self);

#define            sem_down(s) sem_P(s)
#define            sem_wait(s) sem_P(s)
#define            sem_acquire(s) sem_P(s)

#define            sem_up(s) sem_V(s)
#define            sem_signal(s) sem_V(s)
#define            sem_release(s) sem_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
// Number of available resources
static ALWAYS inline int _sem_value(Semaphore* self)
{
	return (self->counter > 0) ? self->counter : 0;
}

// Number of blocked threads
static ALWAYS inline int _sem_blocked(Semaphore* self)
{
	return (self->counter < 0) ? -self->counter : 0;
}

// Idle state ("red" semaphore)? value==0 and blocked==0
static ALWAYS inline int _sem_idle(Semaphore* self)
{
	return self->counter == 0;
}
*/

static inline int sem_init(Semaphore* self, int count)
{
	assert(count >= 0);

	self->counter = count;

	int err;
	if ((err=lck_init(&self->lock)) == thrd_success) {
		if ((err=evt_init(&self->one_can_wakeup, &self->lock)) == thrd_success) {
			return thrd_success;
		} else {
			lck_destroy(&self->lock);
		}
	}
	return err;
}

static inline void sem_destroy(Semaphore* self)
{
	assert(self->counter == 0 ); // idle state

	lck_destroy(&self->lock);
	evt_destroy(&self->one_can_wakeup);
}

//
//Monitor helpers
//
#define ENTER_SEMAPHORE_MONITOR\
	int err_;\
	if ((err_=lck_acquire(&self->lock))!=thrd_success)\
	return err_;

#define LEAVE_SEMAPHORE_MONITOR\
	if ((err_=lck_release(&self->lock))!=thrd_success)\
	return err_;\
	else return thrd_success;

#define CHECK_SEMAPHORE_MONITOR(E)\
	if ((E)!=thrd_success) {\
		lck_release(&self->lock);\
		return (E);\
	}

/*
Decrements the value of semaphore variable by 1. If the new value of 
self->counter is negative, the process executing wait is blocked (i.e.,
added to the semaphore's queue). Otherwise, the process continues execution,
having used a unit of the resource.
*/
static inline int sem_P(Semaphore* self) // P, down, wait, acquire
{
	ENTER_SEMAPHORE_MONITOR

	--self->counter;
	int blocked = self->counter < 0 ? -self->counter : 0;
	if (blocked > 0) { // Do I have to block?
		int err = evt_stay(&self->one_can_wakeup);
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
static inline int sem_V(Semaphore* self) // V, up, signal, release
{
	ENTER_SEMAPHORE_MONITOR

	int blocked = self->counter < 0 ? -self->counter : 0;
	++self->counter;
	if (blocked > 0) { // There are threads blocked?
		int err = evt_signal(&self->one_can_wakeup);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_SEMAPHORE_MONITOR
}

#undef ENTER_SEMAPHORE_MONITOR
#undef LEAVE_SEMAPHORE_MONITOR
#undef CHECK_SEMAPHORE_MONITOR

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
