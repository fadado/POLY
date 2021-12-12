/*
 * Semaphores
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

#include <threads.h>

#include "event.h"

////////////////////////////////////////////////////////////////////////
// Types
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	// counter protected with a mutex
	int   counter;
	mtx_t mutex;
	// signaling wake up
	Event waking;
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

#define ALWAYS __attribute__((always_inline))

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

// Idle state? value==0 and blocked==0
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
	if ((err=mtx_init(&self->mutex, mtx_plain)) == thrd_success) {
		if ((err=evt_init(&self->waking)) == thrd_success) {
			return thrd_success;
		} else {
			mtx_destroy(&self->mutex);
		}
	}
	return err;
}

static inline void sem_destroy(Semaphore* self)
{
	mtx_destroy(&self->mutex);
	evt_destroy(&self->waking);
}

//
//Monitor helpers
//
#define ENTER_SEMAPHORE_MONITOR\
	int err_;\
	if ((err_=mtx_lock(&self->mutex))!=thrd_success)\
	return err_;

#define LEAVE_SEMAPHORE_MONITOR\
	if ((err_=mtx_unlock(&self->mutex))!=thrd_success)\
	return err_;\
	else return thrd_success;

#define CHECK_SEMAPHORE_MONITOR(E)\
	if ((E)!=thrd_success) {\
		mtx_unlock(&self->mutex);\
		return (E);\
	}

/*
Decrements the value of semaphore variable by 1. If the new value of 
self->counter is negative, the process executing wait is blocked (i.e.,
added to the semaphore's queue). Otherwise, the process continues execution,
having used a unit of the resource.
*/
static inline int sem_P(Semaphore* self)
{ // P, down, wait, acquire
	ENTER_SEMAPHORE_MONITOR

	--self->counter;
	int blocked = self->counter < 0 ? -self->counter : 0;
	if (blocked > 0) { // Do I have to block?
		int err = evt_block(&self->waking, &self->mutex);
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
static inline int sem_V(Semaphore* self)
{ // V, up, signal, release
	ENTER_SEMAPHORE_MONITOR

	int blocked = self->counter < 0 ? -self->counter : 0;
	++self->counter;
	if (blocked > 0) { // There are threads blocked?
		int err = evt_signal(&self->waking);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_SEMAPHORE_MONITOR
}

#undef ALWAYS
#undef ASSERT_SEMAPHORE_INVARIANT
#undef ENTER_SEMAPHORE_MONITOR
#undef LEAVE_SEMAPHORE_MONITOR
#undef CHECK_SEMAPHORE_MONITOR

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
