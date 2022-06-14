/* hello.c */
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <stdio.h>
#include "poly/thread.h"

static atomic(bool) done;

THREAD_TYPE(Hello)
	/* empty */
END_TYPE

THREAD_BODY(Hello)
	printf("Hello, world\n");
	done = true;
END_BODY

int main()
{
	run_thread(Hello);

	while (!done) { thread_yield(); }
}

// vim:ai:sw=4:ts=4:syntax=cpp
