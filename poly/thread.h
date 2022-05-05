#ifndef POLY_THREAD_H
#define POLY_THREAD_H

#ifndef POLY_H
#include "POLY.h"
#endif

/*
 * A thin façade renaming on top of C11 type `thrd_t`.
 */

////////////////////////////////////////////////////////////////////////
// Thread interface
////////////////////////////////////////////////////////////////////////

typedef thrd_t Thread;

static Thread   thread_current(void);
static int      thread_detach(Thread thread);
static bool     thread_equal(Thread lhs, Thread rhs);
static void     thread_exit(int result);
static int      thread_fork(int main(void*), void* argument, Thread* this);
static int      thread_join(Thread thread, int *const result);
static int      thread_sleep(Clock duration);
static void     thread_yield(void);

////////////////////////////////////////////////////////////////////////
// Thread implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
thread_fork (int main(void*), void* argument, Thread* this)
{
	return thrd_create(this, main, argument);
}

static ALWAYS inline int
thread_join (Thread thread, int *const result)
{ 
	return thrd_join(thread, result);
}

static ALWAYS inline int
thread_detach (Thread thread)
{ 
	return thrd_detach(thread);
}

static ALWAYS inline bool
thread_equal (Thread lhs, Thread rhs)
{
	return thrd_equal(lhs, rhs);
}

static ALWAYS inline Thread
thread_current (void)
{
	return thrd_current();
}

static ALWAYS inline void
thread_yield (void)
{
	thrd_yield();
}

static ALWAYS inline int
thread_sleep (Clock duration)
{
	// Split `duration` nanoseconds to make an `struct timespec`
	const time_t s  = ns2s(duration);
	const long   ns = duration - s2ns(s);
	return thrd_sleep(&(struct timespec){.tv_sec=s, .tv_nsec=ns}, NULL);
}

static ALWAYS inline void
thread_exit (int result)
{
	thrd_exit(result);
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
