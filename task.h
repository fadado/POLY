/*
 * Task
 *
 */
#ifndef TASK_H
#define TASK_H

#ifndef POLY_H
#include "POLY.h"
#endif

////////////////////////////////////////////////////////////////////////
// Type Event
// Interface
////////////////////////////////////////////////////////////////////////

typedef thrd_t Task; // pure synonym

static inline int  tsk_fork(int(*root)(void*), void* argument, Task* new_task);
static inline int  tsk_join(Task task, int* result);
static inline int  tsk_detach(Task task);
static inline bool tsk_equal(Task task1, Task task2);
static inline Task tsk_self(void);
static inline int  tsk_run(int(*root)(void*), void* argument);
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
	int err = tsk_fork(root, argument, &task);
	if (err != STATUS_SUCCESS) return err;
	return tsk_detach(task);
}

static ALWAYS inline int tsk_fork(int(*root)(void*), void* argument, Task* new_task)
{ return thrd_create(new_task, root, argument); }

static ALWAYS inline int tsk_join(Task task, int* result)
{ return thrd_join(task, result); }

static ALWAYS inline int tsk_detach(Task task)
{ return thrd_detach(task); }

static ALWAYS inline bool tsk_equal(Task task1, Task task2)
{ return thrd_equal(task1, task2); }

static ALWAYS inline Task tsk_self(void)
{ return thrd_current(); }

static ALWAYS inline void tsk_yield(void)
{ thrd_yield(); }

static ALWAYS inline int tsk_sleep(unsigned long long nanoseconds)
{
	time_t s  = ns2s(nanoseconds);
	long   ns = nanoseconds - s2ns(s);
	return thrd_sleep(&(struct timespec){.tv_sec=s, .tv_nsec=ns}, (void*)0);
}

static ALWAYS inline void tsk_exit(int result)
{ thrd_exit(result); }

////////////////////////////////////////////////////////////////////////
// Experimental structures
////////////////////////////////////////////////////////////////////////

#define TASK_SPEC(TASK_NAME,...)\
	struct TASK_NAME;\
	__VA_ARGS__ int TASK_NAME(void*);

#define TASK_BODY(TASK_NAME)\
	struct TASK_NAME {

#define TASK_BEGIN(TASK_NAME)\
	};\
	int TASK_NAME(void* a_) {\
		struct TASK_NAME* self = a_;

#define TASK_END(ignore)\
	}

// TODO: move to future.h when stabilized
#define PROMISE_BEGIN(TASK_NAME)\
	};\
	static int TASK_NAME(void* a_) {\
		struct TASK_NAME* self = ((void**)a_)[0];\
		struct Future* future  = ((void**)a_)[1];

#endif // TASK_H

// vim:ai:sw=4:ts=4:syntax=cpp
