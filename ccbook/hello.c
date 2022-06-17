/* hello.c */
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <stdio.h>
#include "poly/thread.h"

static atomic(bool) done;

THREAD_TYPE(Hello)
	/* empty */
END_TYPE

THREAD_BODY(Hello)
	printf("Hello, world!\n");
	done = true;
END_BODY

int main(void)
{
	int err;

	run_thread(Hello);

	while (!done) { thread_yield(); }

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
