#ifndef SUGAR_H
#define SUGAR_H

////////////////////////////////////////////////////////////////////////
// Declaring and defining tasks
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
    {   /*assert(arg_ != (void*)0);*/\
        struct T const this = *((struct T*)arg_);\
        /* fetch-and-increment atomic global counter*/\
        thread_ID_ = thread_ID_COUNT_++;\
        thread_detach(thread_current());

#define END_BODY\
        return 0;\
    }

////////////////////////////////////////////////////////////////////////
// Spawning threads, futures, filters...
////////////////////////////////////////////////////////////////////////

/*
 *  TASK_TYPE (name)
 *      ...
 *  END_TYPE
 *
 *  TASK_BODY (name)
 *      ...
 *  END_BODY
 */
#define task(T,...)\
    thread_fork(T, &(struct T){__VA_ARGS__}, &(Thread){0})

/*
 *  TASK_TYPE (name)
 *      Channel* input;  // valid also for Port type
 *      Channel* output;
 *      slots
 *      ...
 *  END_TYPE
 *
 *  TASK_BODY (name)
 *      ...
 *  END_BODY
 */
#define filter(T,I,O,...)\
    thread_fork(T, \
        &(struct T){.input=(I), .output=(O)__VA_OPT__(,)__VA_ARGS__},\
        &(Thread){0})

/*
 *  TASK_TYPE (name)
 *      Task* future;
 *      slots
 *      ...
 *  END_TYPE
 *
 *  TASK_BODY (name)
 *      ...
 *  END_BODY
 */
#define future(T,F,...)\
    future_fork(T,\
        &(struct T){.future=(F)__VA_OPT__(,)__VA_ARGS__}, (F))

#endif // vim:ai:sw=4:ts=4:et:syntax=cpp
