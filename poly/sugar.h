#ifndef SUGAR_H
#define SUGAR_H

////////////////////////////////////////////////////////////////////////
// Declaring and defining threads
////////////////////////////////////////////////////////////////////////

/*
 * THREAD (name, [extern | static])
 */
#define THREAD(T,...)\
	struct T;\
	__VA_ARGS__ int T(void*);

/*
 * THREAD_BODY (name)
 * 	type var;
 * 	...
 * THREAD_BEGIN (name)
 * 	code
 * 	...
 * THREAD_END
 */
#define THREAD_BODY(T)\
	struct T {

#define THREAD_BEGIN(T)\
	};\
	int T(void* arg_) {\
		struct T const this = *((struct T*)arg_);\
		thread_detach(thread_current());

#define THREAD_END\
	return 0; }

////////////////////////////////////////////////////////////////////////
// Spawning threads, tasks, filters...
////////////////////////////////////////////////////////////////////////

/*
 * THREAD_BODY (name)
 * 	...
 * THREAD_BEGIN (name)
 * 	...
 * THREAD_END
 */
#define spawn_thread(T,...)\
	thread_fork(T, &(struct T){__VA_ARGS__}, &(Thread){0})

/*
 * THREAD_BODY (name)
 * 	Task* future;
 * 	...
 * THREAD_BEGIN (name)
 * 	...
 * THREAD_END
 */
#define spawn_task(F,T,...)\
	task_fork((T), &(struct T){.future=(F)__VA_OPT__(,)__VA_ARGS__}, (F))

/*
 * THREAD_BODY (name)
 * 	Channel* input;
 * 	Channel* output;
 * 	...
 * THREAD_BEGIN (name)
 * 	...
 * THREAD_END
 */
#define spawn_filter(I,O,T,...)\
	thread_fork(T, \
			&(struct T){.input=(I), .output=(O)__VA_OPT__(,)__VA_ARGS__},\
			&(Thread){0})

#endif // vim:ai:sw=4:ts=4:syntax=cpp
