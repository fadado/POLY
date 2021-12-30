/*
 * Semaphore
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
// Type
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	int   counter;
	Lock  entry;
	Event queue;
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

// Number of blocked threads in the queue
static ALWAYS inline int _sem_length(Semaphore* self)
{
	return (self->counter < 0) ? -self->counter : 0;
}

// Idle state ("red" semaphore)? value==0 and length==0
static ALWAYS inline bool _sem_idle(Semaphore* self)
{
	return self->counter == 0;
}
*/

static inline int sem_init(Semaphore* self, int count)
{
	assert(count >= 0);

	self->counter = count;

	int err;
	if ((err=lck_init(&self->entry)) == STATUS_SUCCESS) {
		if ((err=evt_init(&self->queue, &self->entry)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			lck_destroy(&self->entry);
		}
	}
	return err;
}

static inline void sem_destroy(Semaphore* self)
{
	assert(self->counter == 0 ); // idle state

	evt_destroy(&self->queue);
	lck_destroy(&self->entry);
}

//
//Monitor helpers
//
#define ENTER_SEMAPHORE_MONITOR\
	int err_;\
	if ((err_=lck_acquire(&self->entry))!=STATUS_SUCCESS)\
	return err_;

#define LEAVE_SEMAPHORE_MONITOR\
	if ((err_=lck_release(&self->entry))!=STATUS_SUCCESS)\
	return err_;\
	else return STATUS_SUCCESS;

#define CHECK_SEMAPHORE_MONITOR(E)\
	if ((E)!=STATUS_SUCCESS) {\
		lck_release(&self->entry);\
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
	int length = self->counter < 0 ? -self->counter : 0;
	if (length > 0) { // Do I have to block?
		int err = evt_stay(&self->queue);
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

	int length = self->counter < 0 ? -self->counter : 0;
	++self->counter;
	if (length > 0) { // There are threads blocked?
		int err = evt_signal(&self->queue);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_SEMAPHORE_MONITOR
}

#undef ENTER_SEMAPHORE_MONITOR
#undef LEAVE_SEMAPHORE_MONITOR
#undef CHECK_SEMAPHORE_MONITOR

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
