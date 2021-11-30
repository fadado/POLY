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
////////////////////////////////////////////////////////////////////////

typedef struct {
	short count;
	mtx_t lock;
	cnd_t non_zero;
} Semaphore;

#ifdef DEBUG
#	define ASSERT_SEMAPHORE_INVARIANT\
		assert(0 <= self->count);
#else
#	define ASSERT_SEMAPHORE_INVARIANT
#endif

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

static inline int  sem_init(Semaphore* self, int count);
static inline void sem_destroy(Semaphore* self);
static inline int  sem_P(Semaphore* self);
static inline int  sem_V(Semaphore* self);
#define            sem_acquire(s) sem_P(s)
#define            sem_release(s) sem_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/*
 *
 */
static inline int sem_init(Semaphore* self, int count)
{
	assert(count >= 0);

	self->count = count;
	ASSERT_SEMAPHORE_INVARIANT

	// initialize mutex and conditions
	int err, eN=0;
#	define catch(X)	if ((++eN,err=(X))!=thrd_success) goto onerror

	catch (mtx_init(&self->lock, mtx_plain)); // eN == 1
	catch (cnd_init(&self->non_zero));        // eN == 2

	return thrd_success;
onerror:
#	undef catch
	assert(err != thrd_success);
	if (eN == 2) mtx_destroy(&self->lock);
	return err;
}

/*
 *
 */
static inline void sem_destroy(Semaphore* self)
{
	mtx_destroy(&self->lock);
	cnd_destroy(&self->non_zero);
}

/*
 *
 */
static inline int sem_P(Semaphore* self)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) return err

	catch (mtx_lock(&self->lock));
	while (self->count == 0) {
		if ((err=cnd_wait(&self->non_zero, &self->lock)) != thrd_success) {
			mtx_destroy(&self->lock);
			return err;
		}
	}

	--self->count;
	ASSERT_SEMAPHORE_INVARIANT

	catch (mtx_unlock(&self->lock));

	return thrd_success;
#	undef catch
}

/*
 *
 */
static inline int sem_V(Semaphore* self)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) return err

	catch (mtx_lock(&self->lock));

	++self->count;
	ASSERT_SEMAPHORE_INVARIANT

	if ((err=mtx_unlock(&self->lock)) != thrd_success) {
		cnd_signal(&self->non_zero);
		return err;
	}
	catch (cnd_signal(&self->non_zero));

	return thrd_success;
#	undef catch
}

#undef ASSERT_SEMAPHORE_INVARIANT
//#undef ENTER_SEMAPHORE_MONITOR
//#undef LEAVE_SEMAPHORE_MONITOR

#endif // SEMAPHORE_H

// vim:ai:sw=4:ts=4:syntax=cpp
