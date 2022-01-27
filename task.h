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
// Type Task
// Interface
////////////////////////////////////////////////////////////////////////

typedef thrd_t Task;

static inline Task task_current(void);
static inline int  task_detach(Task task);
static inline bool task_equal(Task task1, Task task2);
static inline void task_exit(int result);
static inline int  task_fork(int(*root)(void*), void* argument, Task* new_task);
static inline int  task_join(Task task, int* result);
static inline int  task_sleep(unsigned long long nanoseconds);
static inline int  task_spawn(int(*root)(void*), void* argument);
static inline void task_yield(void);

// handy macro
#define spawn_task(T,...) task_spawn(T,&(struct T){__VA_ARGS__})

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int task_spawn(int(*root)(void*), void* argument)
{
	Task task;
	int err = task_fork(root, argument, &task);
	if (err != STATUS_SUCCESS) return err;
	return task_detach(task);
}

static ALWAYS inline int task_fork(int(*root)(void*), void* argument, Task* new_task)
{ return thrd_create(new_task, root, argument); }

static ALWAYS inline int task_join(Task task, int* result)
{ return thrd_join(task, result); }

static ALWAYS inline int task_detach(Task task)
{ return thrd_detach(task); }

static ALWAYS inline bool task_equal(Task task1, Task task2)
{ return thrd_equal(task1, task2); }

static ALWAYS inline Task task_current(void)
{ return thrd_current(); }

#define DEFINE_TASK_ID(N)\
	static atomic int _task_id_count;\
	static Task _task_id_vector[N];\
	static int task_id(void) {\
		Task t = task_current();\
		int i, c = _task_id_count;\
		for (i=0; i < c; ++i)\
			if (task_equal(_task_id_vector[i], t))\
				return i;\
		i = _task_id_count++;\
		if (i >= N) panic("looser");\
		_task_id_vector[i] = t;\
		return i;\
	}

static ALWAYS inline void task_yield(void)
{ thrd_yield(); }

static ALWAYS inline int task_sleep(unsigned long long nanoseconds)
{
	time_t s  = ns2s(nanoseconds);
	long   ns = nanoseconds - s2ns(s);
	return thrd_sleep(&(struct timespec){.tv_sec=s, .tv_nsec=ns}, (void*)0);
}

static ALWAYS inline void task_exit(int result)
{ thrd_exit(result); }

////////////////////////////////////////////////////////////////////////
// Experimental structures
////////////////////////////////////////////////////////////////////////

#define TASK_SPEC(TASK,...)\
	struct TASK;\
	__VA_ARGS__ int TASK(void*);

#define TASK_BODY(TASK)\
	struct TASK {

#define TASK_BEGIN(TASK)\
	};\
	int TASK(void* arg_) {\
		struct TASK* this = arg_;

#define TASK_END\
	return 0; }

#endif // TASK_H

// vim:ai:sw=4:ts=4:syntax=cpp
