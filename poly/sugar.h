#ifndef SUGAR_H
#define SUGAR_H

////////////////////////////////////////////////////////////////////////
// Declaring and defining threads
////////////////////////////////////////////////////////////////////////

/*
 * THREAD_TYPE (name, [extern | static])
 */
#define THREAD_TYPE(T,...)\
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
		/*assert(arg_ != (void*)0);*/\
		struct T const this = *((struct T*)arg_);\
		thread_detach(thread_current());

#define THREAD_END\
	return 0; }

////////////////////////////////////////////////////////////////////////
// Spawning threads, futures, filters...
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
#define spawn_future(F,T,...)\
	future_fork((T), &(struct T){.future=(F)__VA_OPT__(,)__VA_ARGS__}, (F))

#define FUTURE_SLOTS\
	Future* future;

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

#define FILTER_SLOTS\
	Channel* input;\
	Channel* output;

#endif // vim:ai:sw=4:ts=4:syntax=cpp
