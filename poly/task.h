#ifndef POLY_TASK_H
#define POLY_TASK_H

#ifndef POLY_THREAD_H
#include "thread.h"
#endif

////////////////////////////////////////////////////////////////////////
// Task: execution context + activation record
////////////////////////////////////////////////////////////////////////

// TASK_ID: 0, 1, ...
static _Thread_local unsigned TASK_ID = 0; // 0 reserved to main
                                           //
// private atomic global counter (provide unique IDs)
static _Atomic unsigned task_ID_COUNT_ = 1;

/*
 *  TASK_TYPE (name [,linkage])
 *      slots
 *      ...
 *  END_TYPE
 */
#define TASK_TYPE(NAME,...)      \
	__VA_ARGS__ int NAME(void*); \
	struct NAME {

#define END_TYPE \
	};

/*
 *  TASK_BODY (name)
 *      code
 *      ...
 *  END_BODY
 */
#define TASK_BODY(NAME)                                 \
	int NAME (void* arg_)                               \
	{                                                   \
		/*assert(arg_ != NULL);*/                       \
		struct NAME const this = *((struct NAME*)arg_); \
		/* fetch-and-increment atomic global counter*/  \
		TASK_ID = task_ID_COUNT_++;                     \
		thread_detach(thread_current());

#define END_BODY  \
		return 0; \
	}

// Run a task, given the name and slots
 #define RUN_task(NAME,...) \
	thread_fork(NAME, &(struct NAME){__VA_ARGS__}, &(Thread){0})

/*
 *  TASK_TYPE (name)
 *      Channel* input;  // Channel or Port
 *      Channel* output;
 *      ...
 *  END_TYPE
 *
 *  TASK_BODY (name)
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
 *  TASK_TYPE (name)
 *      Channel* future;
 *      ...
 *  END_TYPE
 *
 *  TASK_BODY (name)
 *      ...
 *      send result to `future`
 *      ...
 *  END_BODY
 */
#define RUN_promise(T,F,...)                              \
    thread_fork(T,                                        \
        &(struct T){.future=(F)__VA_OPT__(,)__VA_ARGS__}, \
        &(Thread){0})

#endif // vim:ai:sw=4:ts=4:et:syntax=cpp
