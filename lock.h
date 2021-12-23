/*
 * Locks (wrapper to mtx_t)
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef LOCK_H
#define LOCK_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

////////////////////////////////////////////////////////////////////////
// Type Lock
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct { mtx_t mutex; } PlainLock;
typedef PlainLock               Lock;
typedef struct { mtx_t mutex; } TimedLock;
typedef struct { mtx_t mutex; } RecursiveLock;
typedef struct { mtx_t mutex; } TimedRecursiveLock;

union TRANSPARENT lck_ptr {
	mtx_t* mutex;
	PlainLock* _1;
	TimedLock* _2;
	RecursiveLock* _3;
	TimedRecursiveLock* _4;
};

static inline int  lck_init_(union lck_ptr self, int mask);
static inline void lck_destroy(union lck_ptr self);
static inline int  lck_acquire(union lck_ptr self);
static inline int  lck_release(union lck_ptr self);
static inline int  lck_try(union lck_ptr self);
static inline int  lck_watch(union lck_ptr self, struct timespec* ts);

// Deduce mask from lock type
#define lck_init(L) lck_init_((L), \
	_Generic((L),\
		PlainLock*: mtx_plain,\
		TimedLock*: mtx_timed,\
		RecursiveLock*: mtx_plain|mtx_recursive,\
		TimedRecursiveLock*: mtx_timed|mtx_recursive))

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int lck_init_(union lck_ptr self, int mask)
{ return mtx_init(self.mutex, mask); }

static ALWAYS inline void lck_destroy(union lck_ptr self)
{ mtx_destroy(self.mutex); }

static ALWAYS inline int lck_acquire(union lck_ptr self)
{ return mtx_lock(self.mutex); }

static ALWAYS inline int lck_release(union lck_ptr self)
{ return mtx_unlock(self.mutex); }

static ALWAYS inline int lck_try(union lck_ptr self)
{ return mtx_trylock(self.mutex); }

static ALWAYS inline int lck_watch(union lck_ptr self, struct timespec* ts)
{ return mtx_timedlock(self.mutex, ts); }

#endif // LOCK_H

// vim:ai:sw=4:ts=4:syntax=cpp
