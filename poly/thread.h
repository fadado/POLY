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

////////////////////////////////////////////////////////////////////////
// ADA style
////////////////////////////////////////////////////////////////////////

// THREAD_ID: 0, 1, ...
static _Thread_local unsigned THREAD_ID = 0; // 0 reserved to main

// private atomic global counter (provide unique IDs)
static _Atomic unsigned thread_ID_COUNT_ = 1;

/*
 *  THREAD_TYPE (name [,linkage])
 *      slots
 *      entries
 *      ...
 *  END_TYPE
 */
#define THREAD_TYPE(NAME,...)    \
    __VA_ARGS__ int NAME(void*); \
    struct NAME {

#define END_TYPE \
    };

/*
 *  THREAD_BODY (name)
 *      code
 *      ...
 *  END_BODY
 */
#define THREAD_BODY(NAME)                               \
    int NAME (void* arg_)                               \
    {                                                   \
        /*assert(arg_ != NULL);*/                       \
        struct NAME const this = *((struct NAME*)arg_); \
        /* fetch-and-increment atomic global counter*/  \
        THREAD_ID = thread_ID_COUNT_++;                 \
        thread_detach(thread_current());

#define END_BODY  \
        return 0; \
    }

// Run a thread, given the name and slots
 #define RUN_thread(NAME,...) \
    thread_fork(NAME, &(struct NAME){__VA_ARGS__}, &(Thread){0})

/*
 *  THREAD_TYPE (name)
 *      Channel* input;  // Channel or Port
 *      Channel* output;
 *      ...
 *  END_TYPE
 *
 *  THREAD_BODY (name)
 *      ...
 *      receive from `input` and send to `output`
 *      ...
 *  END_BODY
 */
#define RUN_filter(T,I,O,...)                                         \
    thread_fork(T,                                                    \
        &(struct T){.input=(I), .output=(O)__VA_OPT__(,)__VA_ARGS__}, \
        &(Thread){0})

/*
 *  THREAD_TYPE (name)
 *      Channel* future;
 *      ...
 *  END_TYPE
 *
 *  THREAD_BODY (name)
 *      ...
 *      send result to `future`
 *      ...
 *  END_BODY
 */
#define RUN_promise(T,F,...)                              \
    thread_fork(T,                                        \
        &(struct T){.future=(F)__VA_OPT__(,)__VA_ARGS__}, \
        &(Thread){0})

#endif // vim:ai:sw=4:ts=4:syntax=cpp
