#ifndef SUGAR_H
#define SUGAR_H

////////////////////////////////////////////////////////////////////////
// Defining threads
////////////////////////////////////////////////////////////////////////

#define THREAD_SPEC(T,...)\
	struct T;\
	__VA_ARGS__ int T(void*);

#define THREAD_BODY(T)\
	struct T {

#define THREAD_BEGIN(T)\
	};\
	int T(void* arg_) {\
		struct T const this = *((struct T*)arg_);

#define THREAD_END\
	return 0; }

////////////////////////////////////////////////////////////////////////
// Spawning threads
////////////////////////////////////////////////////////////////////////

// require poly/thread.h
#define spawn_thread(T,...)\
	thread_spawn(T, &(struct T){__VA_ARGS__})

// require poly/task.h
#define spawn_task(F,R,...)\
	task_spawn(F, R,&(struct R){.future=F __VA_OPT__(,)__VA_ARGS__})

// require poly/thread.h and poly/passing/channel.h
#define spawn_filter(I,O,T,...)\
	thread_spawn(T, &(struct T){.input=I,.output=O __VA_OPT__(,)__VA_ARGS__ })

#endif // vim:ai:sw=4:ts=4:syntax=cpp
