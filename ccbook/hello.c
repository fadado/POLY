/* hello.c */
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <stdio.h>
#include "poly/thread.h"

typedef atomic(bool) Flag;

struct Hello {
	THREAD_TYPE
};

static Flag done = 0;

int Hello(void* data)
{
	THREAD_BODY (Hello, data)

	printf("Hello, world!\n");

	done = 1;

	END_BODY
}

int main(void)
{
	int err;

	run_thread(Hello);

	while (!done) { thread_yield(); }

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
