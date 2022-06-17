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
 *  THREAD_TYPE (name)
 *      slots
 *  END_TYPE
 * =>
 *  int name_body(void*);
 *  struct name_type { slots };
 */
#define THREAD_TYPE(NAME)   \
    int NAME##_body(void*); \
    struct NAME##_type {

#define END_TYPE \
    }; // struct end

/*
 *  THREAD_BODY (name)
 *      code
 *  END_BODY
 * =>
 *  int name_body(void* arg_)
 *  {
 *      struct name_type const this = *((struct name_type*)arg_);
 *      THREAD_ID = THREAD_ID_COUNT_++;
 *      thread_detach(thread_current());
 *      ...
 *      code
 *      ...
 *      return STATUS_SUCCESS;
 *  }
 */
#define THREAD_BODY(NAME)                \
    int NAME##_body (void* arg_)         \
    {                                    \
        /*assert(arg_ != NULL);*/        \
        struct NAME##_type const this = *((struct NAME##_type*)arg_); \
        /* fetch-and-increment atomic global counter*/ \
        THREAD_ID = THREAD_ID_COUNT_++;  \
        thread_detach(thread_current());

#define END_BODY                \
        return STATUS_SUCCESS;  \
    } // function end

/*
 * Run a thread, given the thread type name and slots.
 *
 *  run_thread (name, slots...);
 * =>
 *  thread_create(&(Thread){0}, name_body, &(struct name_type){slots})
 *
 * Slots syntax:
 *
 *  .s1=v, .s2=v, ...
 */
#define run_thread(NAME,...)                                            \
do {                                                                    \
    struct NAME##_type data_ = {__VA_ARGS__};                           \
    int const err_ = thread_create(&(Thread){0}, NAME##_body, &data_);  \
	if (err_ != STATUS_SUCCESS) panic("cannot start thread");           \
	thread_sleep(ms2ns(1));                                             \
} while (0)

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
#define run_filter(T,I,O,...) \
    run_thread(T, .input=(I), .output=(O) __VA_OPT__(,)__VA_ARGS__)

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
#define run_promise(T,F,...) \
    run_thread(T, .future=(F) __VA_OPT__(,)__VA_ARGS__)

#endif // vim:ai:sw=4:ts=4:syntax=cpp
