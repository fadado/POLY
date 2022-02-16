#ifndef SPINNER_H
#define SPINNER_H

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
static int    thread_join(Thread thread, int* result);
static int    thread_sleep(Time duration);
static int    thread_spawn(int main(void*), void* argument);
static void   thread_yield(void);

// handy macro
#define spawn_thread(T,...)\
	thread_spawn(T, &(struct T){__VA_ARGS__})

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
thread_fork (int main(void*), void* argument, Thread* thread)
{
	return thrd_create(thread, main, argument);
}

static ALWAYS inline int
thread_join (Thread thread, int* result)
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
// Extensions to C11 API
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

#define DEFINE_THREAD_ID(N)\
	static atomic(unsigned) _thread_id_count;\
	static Thread _thread_id_vector[N];\
	static unsigned thread_id(void) {\
		Thread t = thread_current();\
		unsigned i, c = _thread_id_count;\
		for (i=0; i < c; ++i)\
			if (thread_equal(_thread_id_vector[i], t))\
				return i;\
		i = _thread_id_count++;\
		if (i >= N) panic("looser");\
		_thread_id_vector[i] = t;\
		return i;\
	}

////////////////////////////////////////////////////////////////////////
// Macros to define threads, tasks, filters...
////////////////////////////////////////////////////////////////////////

#define THREAD_SPEC(T,...)\
	struct T;\
	__VA_ARGS__ int T(void*);

#define THREAD_BODY(T)\
	struct T {

#define THREAD_BEGIN(T)\
	};\
	int T(void* arg_) {\
		struct T const this = *((struct T*)arg_);

#define THREAD_END\
	return 0; }

#endif // vim:ai:sw=4:ts=4:syntax=cpp
