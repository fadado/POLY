/* printer1.c */
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <stdio.h>
#include "poly/thread.h"
#include "poly/passing/entry.h"
#include "poly/passing/interface.h"

static atomic(bool) done;

THREAD_TYPE(Printer1) END_TYPE

THREAD_BODY(Printer1)
	printf("Hi from printer thread\n");
	done = true;
END_BODY

int main()
{
	printf("Hi from main thread\n");
	run_thread(Printer1);
	printf("Bye from main thread\n");

	while (!done) { thread_yield(); }

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
