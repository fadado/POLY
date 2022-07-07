/* printer1.c */
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <stdio.h>
#include "poly/thread.h"

typedef atomic(bool) Flag;

struct Printer1 {
	THREAD_TYPE
};

static Flag done = 0;

int Printer1(void* data)
{
	THREAD_BODY (Printer1, data)

	printf("Hi from printer thread\n");

	done = 1;

	END_BODY
}

int main()
{
	printf("Hi from main thread\n");
	run_thread(Printer1);
	printf("Bye from main thread\n");

	while (!done) { thread_yield(); }

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
