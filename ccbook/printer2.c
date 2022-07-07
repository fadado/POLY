/* printer2.c */

#include <stdio.h>
#include "poly/thread.h"

typedef atomic(short) Counter;

struct Printer2 {
	THREAD_TYPE
	int i;
};

static Counter done = 0;

int Printer2(void* data)
{
	THREAD_BODY (Printer2, data)

	printf("Hi from printer thread: %d\n", this.i);
	++done;

	END_BODY
}

int main()
{
	printf("Hi from main thread\n");
	run_thread(Printer2, .i=1);
	run_thread(Printer2, .i=2);
	printf("Bye from main thread\n");

	while (done < 2) { thread_yield(); }

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
