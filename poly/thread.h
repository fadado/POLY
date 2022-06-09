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
static int      thread_create(Thread* this, int main(void*), void* argument);
static bool     thread_equal(Thread lhs, Thread rhs);
static void     thread_exit(int result);
static int      thread_join(Thread thread, int *const result);
static int      thread_sleep(Clock duration);
static void     thread_yield(void);

////////////////////////////////////////////////////////////////////////
// Thread implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
thread_create (Thread* this, int main(void*), void* argument)
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
// Ada style
////////////////////////////////////////////////////////////////////////

// THREAD_ID: 0, 1, ... (0 reserved to main)
static _Thread_local unsigned   THREAD_ID = 0;
// private atomic global counter (provide unique IDs)
static _Atomic unsigned         THREAD_ID_COUNT_ = 1;

/*
 *  THREAD_TYPE (name [,linkage])
 *      parameters
 *      entries
 *  END_TYPE
 * =>
 *  int name(void*);
 *  struct name { parameters entries };
 */
#define THREAD_TYPE(NAME,...)    \
    __VA_ARGS__ int NAME(void*); \
    struct NAME {

#define END_TYPE \
    };

/*
 *  THREAD_BODY (name)
 *      code
 *  END_BODY
 * =>
 *  int name(void* arg_)
 *  {
 *      struct name const this = *((struct name*)arg_);
 *      THREAD_ID = THREAD_ID_COUNT_++;
 *      thread_detach(thread_current());
 *      ...
 *      code
 *      ...
 *      return STATUS_SUCCESS;
 *  }
 */
#define THREAD_BODY(NAME)                               \
    int NAME (void* arg_)                               \
    {                                                   \
        /*assert(arg_ != NULL);*/                       \
        struct NAME const this = *((struct NAME*)arg_); \
        /* fetch-and-increment atomic global counter*/  \
        THREAD_ID = THREAD_ID_COUNT_++;                 \
        thread_detach(thread_current());

#define END_BODY                \
        return STATUS_SUCCESS;  \
    }

/*
 * Run a thread, given the name and parameters.
 *
 *  create (name, parameters...);
 * =>
 *  thread_create(&(Thread){0}, name, &(struct name){parameters})
 *
 * Parameters form:
 *
 *  .p1=v, .p2=v, ...
 */
#define create(NAME,...) \
    thread_create(&(Thread){0}, NAME, &(struct NAME){__VA_ARGS__})

#endif // vim:ai:sw=4:ts=4:syntax=cpp
