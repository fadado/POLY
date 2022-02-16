#ifndef LOCK_H
#define LOCK_H

#ifndef POLY_H
#include "POLY.h"
#endif

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct { mtx_t mutex; } PlainLock;
typedef struct { mtx_t mutex; } TimedLock;
typedef struct { mtx_t mutex; } RecursiveLock;
typedef struct { mtx_t mutex; } TimedRecursiveLock;

// handy aliases
typedef PlainLock               Lock;
typedef TimedRecursiveLock      RecursiveTimedLock;

union TRANSPARENT Lock {
	mtx_t* mutex;
	PlainLock* _1;
	TimedLock* _2;
	RecursiveLock* _3;
	TimedRecursiveLock* _4;
};

static int  lock_acquire(union Lock this);
static void lock_destroy(union Lock this);
static int  lock_init(union Lock this, unsigned mask);
static int  lock_release(union Lock this);
static int  lock_try(union Lock this);
static int  lock_try_for(union Lock this, Time duration);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
lock_init (union Lock this, unsigned mask)
{
	return mtx_init(this.mutex, mask);
}

// Deduce mask from lock type, and hide lock_init function
#define lock_init(LOCK) lock_init((LOCK), \
	_Generic((LOCK),\
		PlainLock*: mtx_plain,\
		TimedLock*: mtx_timed,\
		RecursiveLock*: mtx_plain|mtx_recursive,\
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
lock_try_for (union Lock this, Time duration)
{
	Time   t  = now() + duration;
	time_t s  = ns2s(t);
	long   ns = t - s2ns(s);
	return mtx_timedlock(this.mutex, &(struct timespec){.tv_sec=s, .tv_nsec=ns});
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
