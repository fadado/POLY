/*
 * Task
 *
 */
#ifndef TASK_H
#define TASK_H

#ifndef POLY_H
#error To conduct the choir I need "poly.h"!
#endif

////////////////////////////////////////////////////////////////////////
// Type Event
// Interface
////////////////////////////////////////////////////////////////////////

typedef thrd_t Task;

static inline int  tsk_run(int(*root)(void*), void* argument);
static inline int  tsk_fork(int(*root)(void*), void* argument, Task* new_task);
static inline int  tsk_join(Task task);
static inline int  tsk_detach(Task task);
static inline int  tsk_equal(Task task1, Task task2);
static inline Task tsk_self(void);
static inline void tsk_yield(void);
static inline int  tsk_sleep(unsigned long long nanoseconds);
static inline void tsk_exit(int result);

// handy macro
#define tsk_spawn(T,...) tsk_run(T, &(struct T){__VA_ARGS__})

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int tsk_run(int(*root)(void*), void* argument)
{
	Task task;
	int err = thrd_create(&task, root, argument);
	if (err != STATUS_SUCCESS) return err;
	return thrd_detach(task);
}

static ALWAYS inline int tsk_fork(int(*root)(void*), void* argument, Task* new_task)
{ return thrd_create(new_task, root, argument); }

static ALWAYS inline int tsk_join(Task task)
{ return thrd_join(task, (void*)0); }

static ALWAYS inline int tsk_detach(Task task)
{ return thrd_detach(task); }

static ALWAYS inline int tsk_equal(Task task1, Task task2)
{ return thrd_equal(task1, task2); }

static ALWAYS inline Task tsk_self(void)
{ return thrd_current(); }

static ALWAYS inline void tsk_yield(void)
{ thrd_yield(); }

static ALWAYS inline int tsk_sleep(unsigned long long nanoseconds)
{
	time_t s = nanoseconds/1000000000ULL;
	long   n = nanoseconds%1000000000ULL;
	return thrd_sleep(&(struct timespec){.tv_sec=s, .tv_nsec=n}, (void*)0);
}

static ALWAYS inline void tsk_exit(int result)
{ thrd_exit(result); }

#endif // TASK_H

// vim:ai:sw=4:ts=4:syntax=cpp
