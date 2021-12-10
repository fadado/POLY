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

////////////////////////////////////////////////////////////////////////
// Types
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	// counter protected with a lock
	int   counter;
	mtx_t lock;
	// non-spurious condition
	int   waking;
	cnd_t queue;
} Semaphore;

// Special count values to initialize semaphores
enum {
	SEMAPHORE_DOWN, // semaphore as event
	SEMAPHORE_UP    // semaphore as lock
};

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
static ALWAYS inline int _sem_avail(Semaphore* self)
{
	return (self->counter > 0) ? self->counter : 0;
}

// Number of blocked threads
static ALWAYS inline int _sem_waiting(Semaphore* self)
{
	return (self->counter < 0) ? -self->counter : 0;
}

// Idle state?
static ALWAYS inline int _sem_idle(Semaphore* self)
{
	return self->counter == 0;
}
*/

/*
 *
 */
static inline int sem_init(Semaphore* self, int count)
{
	assert(count >= 0);
	self->counter = count;
	self->waking = 0;

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
	return err_;

#define LEAVE_SEMAPHORE_MONITOR\
	if ((err_=mtx_unlock(&self->lock))!=thrd_success)\
	return err_;\
	else return thrd_success;

#define CHECK_SEMAPHORE_MONITOR(E)\
	if ((E)!=thrd_success) {\
		mtx_unlock(&self->lock);\
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
		do {
			int err = cnd_wait(&self->queue, &self->lock);
			CHECK_SEMAPHORE_MONITOR (err)
		} while (self->waking == 0);
		--self->waking;
		assert(self->waking >= 0);
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
		++self->waking;
		int err = cnd_signal(&self->queue);
		CHECK_SEMAPHORE_MONITOR (err)
	}

	LEAVE_SEMAPHORE_MONITOR
}

#undef ALWAYS
#undef ASSERT_SEMAPHORE_INVARIANT
#undef ENTER_SEMAPHORE_MONITOR
#undef LEAVE_SEMAPHORE_MONITOR
#undef BREAK_SEMAPHORE_MONITOR

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
