#ifndef THREAD_H
#define THREAD_H

/* Module parameters:
 *     THREAD_ID_SIZE
 */

#ifndef POLY_H
#include "POLY.h"
#endif

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef thrd_t Thread;

static Thread   thread_current(void);
static int      thread_detach(Thread thread);
static bool     thread_equal(Thread lhs, Thread rhs);
static void     thread_exit(int result);
static int      thread_fork(int main(void*), void* argument, Thread* this);
static int      thread_join(Thread thread, int *const result);
static int      thread_sleep(Time duration);
static void     thread_yield(void);

////////////////////////////////////////////////////////////////////////
// Implementation
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
thread_sleep (Time duration)
{
	const time_t s  = ns2s(duration);
	const long   ns = duration - s2ns(s);
	return thrd_sleep(&(struct timespec){.tv_sec=s, .tv_nsec=ns}, (struct timespec*)0);
}

static ALWAYS inline void
thread_exit (int result)
{
	thrd_exit(result);
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
