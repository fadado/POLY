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

static Thread thread_current(void);
static int    thread_detach(Thread thread);
static bool   thread_equal(Thread lhs, Thread rhs);
static void   thread_exit(int result);
static int    thread_fork(int main(void*), void* argument, Thread* thread);
static int    thread_join(Thread thread, int *const result);
static int    thread_sleep(Time duration);
static int    thread_spawn(int main(void*), void* argument);
static void   thread_yield(void);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
thread_fork (int main(void*), void* argument, Thread* thread)
{
	return thrd_create(thread, main, argument);
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
	time_t s  = ns2s(duration);
	long   ns = duration - s2ns(s);
	return thrd_sleep(&(struct timespec){.tv_sec=s, .tv_nsec=ns}, (void*)0);
}

static ALWAYS inline void
thread_exit (int result)
{
	thrd_exit(result);
}

////////////////////////////////////////////////////////////////////////
// Extensions to C11 thrd_t API
////////////////////////////////////////////////////////////////////////

static inline int
thread_spawn (int main(void*), void* argument)
{
	Thread thread;
	int err = thread_fork(main, argument, &thread);
	if (err != STATUS_SUCCESS) return err;
	// TODO: thread_yield(): ensure main has been called???
	return thread_detach(thread);
}

#ifdef THREAD_ID_SIZE

static_assert(THREAD_ID_SIZE > 0);

static atomic(unsigned) thread_id_count_ = {0};
static Thread           thread_id_vector_[THREAD_ID_SIZE] = {0};

static unsigned thread_id(void)
{
	Thread current = thread_current();
	unsigned i, count = thread_id_count_;
	for (i=0; i < count; ++i)
		if (thread_equal(thread_id_vector_[i], current))
			return i;
	i = thread_id_count_++;
	if (i >= THREAD_ID_SIZE) panic("looser");
	thread_id_vector_[i] = current;
	return i;
}

#endif // THREAD_ID_SIZE

#endif // vim:ai:sw=4:ts=4:syntax=cpp
