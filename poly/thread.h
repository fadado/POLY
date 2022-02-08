#ifndef SPINNER_H
#define SPINNER_H

#ifndef POLY_H
#include "POLY.h"
#endif

////////////////////////////////////////////////////////////////////////
// Type Thread
// Interface
////////////////////////////////////////////////////////////////////////

typedef thrd_t Thread;

static inline Thread thread_current(void);
static inline int    thread_detach(Thread thread);
static inline bool   thread_equal(Thread lhs, Thread rhs);
static inline void   thread_exit(int result);
static inline int    thread_fork(int(*root)(void*), void* argument, Thread* thread);
static inline int    thread_join(Thread thread, int* result);
static inline int    thread_sleep(Time duration);
static inline void   thread_yield(void);

// handy macro
static inline int thread_spawn(int(*root)(void*), void* argument);

#define spawn_thread(T,...) thread_spawn(T,&(struct T){__VA_ARGS__})

////////////////////////////////////////////////////////////////////////
// Thread implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int thread_fork(int(*root)(void*), void* argument, Thread* thread)
{ return thrd_create(thread, root, argument); }

static ALWAYS inline int thread_join(Thread thread, int* result)
{ return thrd_join(thread, result); }

static ALWAYS inline int thread_detach(Thread thread)
{ return thrd_detach(thread); }

static ALWAYS inline bool thread_equal(Thread lhs, Thread rhs)
{ return thrd_equal(lhs, rhs); }

static ALWAYS inline Thread thread_current(void)
{ return thrd_current(); }

static ALWAYS inline void thread_yield(void)
{ thrd_yield(); }

static ALWAYS inline int thread_sleep(Time duration)
{
	time_t s  = ns2s(duration);
	long   ns = duration - s2ns(s);
	return thrd_sleep(&(struct timespec){.tv_sec=s, .tv_nsec=ns}, (void*)0);
}

static ALWAYS inline void thread_exit(int result)
{ thrd_exit(result); }

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int thread_spawn(int(*root)(void*), void* argument)
{
	Thread thread;
	int err = thread_fork(root, argument, &thread);
	if (err != STATUS_SUCCESS) return err;
	// TODO: thread_yield(): ensure root has been called???
	return thread_detach(thread);
}

#define DEFINE_THREAD_ID(N)\
	static atomic(int) _thread_id_count;\
	static Thread _thread_id_vector[N];\
	static int thread_id(void) {\
		Thread t = thread_current();\
		int i, c = _thread_id_count;\
		for (i=0; i < c; ++i)\
			if (thread_equal(_thread_id_vector[i], t))\
				return i;\
		i = _thread_id_count++;\
		if (i >= N) panic("looser");\
		_thread_id_vector[i] = t;\
		return i;\
	}

////////////////////////////////////////////////////////////////////////
// Experimental structures
////////////////////////////////////////////////////////////////////////

#define THREAD_SPEC(T,...)\
	struct T;\
	__VA_ARGS__ int T(void*);

#define THREAD_BODY(T)\
	struct T {

#define THREAD_BEGIN(T)\
	};\
	int T(void* arg_) {\
		struct T this = *((struct T*)arg_);

#define THREAD_END\
	return 0; }

// vim:ai:sw=4:ts=4:syntax=cpp
#endif // SPINNER_H
