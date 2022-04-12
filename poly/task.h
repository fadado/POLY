#ifndef TASK_H
#define TASK_H

////////////////////////////////////////////////////////////////////////
// Task: a execution context + activation record
////////////////////////////////////////////////////////////////////////

/*
 *  TASK_TYPE (name [,linkage])
 *      slots
 *      ...
 *  END_TYPE
 */
#define TASK_TYPE(T,...)\
    __VA_ARGS__ int T(void*);\
    struct T {

#define END_TYPE\
    };

/*
 *  TASK_BODY (name)
 *      code
 *      ...
 *  END_BODY
 */
#define TASK_BODY(T)\
    int T (void* arg_)\
    {   /*assert(arg_ != NULL);*/\
        struct T const this = *((struct T*)arg_);\
        /* fetch-and-increment atomic global counter*/\
        TASK_ID = task_ID_COUNT_++;\
        thread_detach(thread_current());

#define END_BODY\
        return 0;\
    }

// atomic global counter (provide unique IDs)
// TASK_ID: 0, 1, ...
static _Atomic       unsigned task_ID_COUNT_ = 1;
static _Thread_local unsigned TASK_ID = 0; // 0 reserved to main

/*
 *  TASK_TYPE (name)
 *      ...
 *  END_TYPE
 *
 *  TASK_BODY (name)
 *      ...
 *  END_BODY
 */
#define RUN_task(T,...)\
    thread_fork(T, &(struct T){__VA_ARGS__}, &(Thread){0})

#endif // vim:ai:sw=4:ts=4:et:syntax=cpp
