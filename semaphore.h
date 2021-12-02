/*
 * Semaphores
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <threads.h>

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

//#define LIFO_SPURIOUS_WAKEUPS

////////////////////////////////////////////////////////////////////////
// Types
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct {
	int   count;
#ifndef LIFO_SPURIOUS_WAKEUPS
	int   wakeups;
#endif
	mtx_t lock;
	cnd_t queue;
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

//#undef ASSERT_SEMAPHORE_INVARIANT

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#define ALWAYS __attribute__((always_inline))

/*

// Number of vailable resources
static ALWAYS inline int _sem_avail(Semaphore* self)
{
	return (self->count > 0) ? self->count : 0;
}

// Number of blocked threads
static ALWAYS inline int _sem_blocked(Semaphore* self)
{
	return (self->count < 0) ? -self->count : 0;
}

*/

/*
 *
 */
static inline int sem_init(Semaphore* self, int count)
{
	assert(count >= 0);
	self->count = count;
#ifndef LIFO_SPURIOUS_WAKEUPS
	self->wakeups = 0;
#endif

	int err;
	if ((err=mtx_init(&self->lock, mtx_plain)) != thrd_success) {
		return err;
	}
	if ((err=cnd_init(&self->queue)) != thrd_success) {
		mtx_destroy(&self->lock);
		return err;
	}
	return thrd_success;
}

/*
 *
 */
static inline void sem_destroy(Semaphore* self)
{
	mtx_destroy(&self->lock);
	cnd_destroy(&self->queue);
}


//
//Monitor helpers
//
#define ENTER_SEMAPHORE_MONITOR\
	int err_;\
	if ((err_=mtx_lock(&self->lock))!=thrd_success)\
	return err_

#define LEAVE_SEMAPHORE_MONITOR\
	if ((err_=mtx_unlock(&self->lock))!=thrd_success)\
	return err_;\
	else return thrd_success

#define BREAK_SEMAPHORE_MONITOR(E)\
	mtx_unlock(&self->lock);\
	return (E)

/*
Decrements the value of semaphore variable by 1. If the new value of 
self->count is negative, the process executing wait is blocked (i.e.,
added to the semaphore's queue). Otherwise, the process continues execution,
having used a unit of the resource.
*/
static inline int sem_P(Semaphore* self)
{ // P, down, wait, acquire
	ENTER_SEMAPHORE_MONITOR;

	--self->count;
	if (self->count < 0) {
#ifdef LIFO_SPURIOUS_WAKEUPS
		int top = self->count;
		while (self->count <= top) {
			int err = cnd_wait(&self->queue, &self->lock);
			if (err != thrd_success) {
				BREAK_SEMAPHORE_MONITOR(err);
			}
		}
#else
		do {
			int err = cnd_signal(&self->queue);
			if (err != thrd_success) {
				BREAK_SEMAPHORE_MONITOR(err);
			}
		} while (self->wakeups < 1);
		--self->wakeups;
#endif
	}

	LEAVE_SEMAPHORE_MONITOR;
}

/*
Increments the value of semaphore variable by 1. After the increment, if the
value is negative or zero (meaning there are processes waiting for a
resource), it transfers a blocked process from the semaphore's waiting queue
to the ready queue.
*/
static inline int sem_V(Semaphore* self)
{ // V, up, signal, release
	ENTER_SEMAPHORE_MONITOR;

	++self->count;
	if (self->count <= 0) {
#ifdef LIFO_SPURIOUS_WAKEUPS
		int err = cnd_broadcast(&self->queue);
#else
		++self->wakeups;
		int err = cnd_signal(&self->queue);
#endif
		if (err != thrd_success) {
			BREAK_SEMAPHORE_MONITOR(err);
		}
	}

	LEAVE_SEMAPHORE_MONITOR;
}

#undef ALWAYS
#undef ENTER_SEMAPHORE_MONITOR
#undef LEAVE_SEMAPHORE_MONITOR
#undef BREAK_SEMAPHORE_MONITOR


#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
