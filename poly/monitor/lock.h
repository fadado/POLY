#ifndef POLY_LOCK_H
#define POLY_LOCK_H

#ifndef POLY_H
#include "../POLY.h"
#endif

/*
 * A façade on top of C11 type `mtx_t`.
 */

////////////////////////////////////////////////////////////////////////
// Lock interface
////////////////////////////////////////////////////////////////////////

/*
 *  PlainLock l; // or TimedLock, RecursiveLock, TimedRecursiveLock
 *  lock_init(&l)
 *
 * expands to
 *
 *  lock_init(&l, mask);
 *
 * with `mask` deduced from lock type
 *
 */

typedef struct { mtx_t mutex; } PlainLock;
typedef struct { mtx_t mutex; } TimedLock;
typedef struct { mtx_t mutex; } RecursiveLock;
typedef struct { mtx_t mutex; } TimedRecursiveLock;

union POLY_TRANSPARENT Lock {
	mtx_t* mutex;
	PlainLock* _1;
	TimedLock* _2;
	RecursiveLock* _3;
	TimedRecursiveLock* _4;
};

// handy aliases
typedef PlainLock               Lock;
typedef TimedRecursiveLock      RecursiveTimedLock;

static int  lock_acquire(union Lock this);
static void lock_destroy(union Lock this);
// macro:   lock_init(union Lock this)
static int  lock_init(union Lock this, unsigned mask);
static int  lock_release(union Lock this);
static int  lock_try(union Lock this);
static int  lock_try_for(union Lock this, Clock duration);

////////////////////////////////////////////////////////////////////////
// Lock implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
lock_init (union Lock this, unsigned mask)
{
	return mtx_init(this.mutex, mask);
}

// Deduce mask from lock type, and calls lock_init function
#define lock_init(LOCK) lock_init((LOCK),        \
	_Generic((LOCK),                             \
		PlainLock*: mtx_plain,                   \
		TimedLock*: mtx_timed,                   \
		RecursiveLock*: mtx_plain|mtx_recursive, \
		TimedRecursiveLock*: mtx_timed|mtx_recursive))

static ALWAYS inline void
lock_destroy (union Lock this)
{
	mtx_destroy(this.mutex);
}

static ALWAYS inline int
lock_acquire (union Lock this)
{
	return mtx_lock(this.mutex);
}

static ALWAYS inline int
lock_release (union Lock this)
{
	return mtx_unlock(this.mutex);
}

static ALWAYS inline int
lock_try (union Lock this)
{
	return mtx_trylock(this.mutex);
}

static inline int
lock_try_for (union Lock this, Clock duration)
{
	const Clock  t  = now() + duration; // Clock ticks are nanoseconds
	const time_t s  = ns2s(t);
	const long   ns = t - s2ns(s);
	return mtx_timedlock(this.mutex, &(struct timespec){.tv_sec=s, .tv_nsec=ns});
}

////////////////////////////////////////////////////////////////////////
// Monitor helper macros
////////////////////////////////////////////////////////////////////////

#ifndef POLY_MONITOR_H
#include "MONITOR.h"
#endif

#endif // vim:ai:sw=4:ts=4:syntax=cpp
