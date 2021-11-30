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

typedef struct {
	int   count;
	mtx_t lock;
	cnd_t queue;
} Semaphore;

static inline int  sem_init(Semaphore* self, int count);
static inline void sem_destroy(Semaphore* self);
static inline int  sem_P(Semaphore* self);
static inline int  sem_V(Semaphore* self);

#define            sem_down(s) sem_P(s)
#define            sem_acquire(s) sem_P(s)
#define            sem_wait(s) sem_P(s)

#define            sem_up(s) sem_V(s)
#define            sem_signal(s) sem_V(s)
#define            sem_release(s) sem_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#define ALWAYS __attribute__((always_inline))

/*
static ALWAYS inline int _sem_avail(Semaphore* self)
{
	return (self->count > 0) ? self->count : 0;
}

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

	int err;
	if ((err=mtx_init(&self->lock, mtx_plain)) != thrd_success) {
		return err;
	}
	if ((err=cnd_init(&self->queue)) != thrd_success) {
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

/*
Decrements the value of semaphore variable by 1. If the new value of the
semaphore variable is negative, the process executing wait is blocked (i.e.,
added to the semaphore's queue). Otherwise, the process continues execution,
having used a unit of the resource.
*/
static inline int sem_P(Semaphore* self)
{
	int err;

	if ((err=mtx_lock(&self->lock)) != thrd_success) {
		return err;
	}

	--self->count;
	if (self->count < 0) {
		int c = self->count;
		while (self->count <= c) {
			// TODO: detect spurious wakeup!!!
			if ((err=cnd_wait(&self->queue, &self->lock)) != thrd_success) {
				mtx_unlock(&self->lock);
				return err;
			}
		}
	}

	if ((err=mtx_unlock(&self->lock)) != thrd_success) {
		return err;
	}

	return thrd_success;
}

/*
Increments the value of semaphore variable by 1. After the increment, if the
pre-increment value was negative (meaning there are processes waiting for a
resource), it transfers a blocked process from the semaphore's waiting queue
to the ready queue.
*/
static inline int sem_V(Semaphore* self)
{
	int err;

	if ((err=mtx_lock(&self->lock)) != thrd_success) {
		return err;
	}

	++self->count;
	if (self->count <= 0) {
		if ((err=cnd_signal(&self->queue)) != thrd_success) {
			mtx_unlock(&self->lock);
			return err;
		}
	}

	if ((err=mtx_unlock(&self->lock)) != thrd_success) {
		return err;
	}

	return thrd_success;
}

#undef ALWAYS

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
