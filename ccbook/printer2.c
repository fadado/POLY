/* printer2.c */
#pragma GCC diagnostic ignored "-Wunused-function"

#include <stdio.h>
#include "poly/thread.h"
#include "poly/passing/entry.h"
#include "poly/passing/interface.h"

static atomic(int) done;

INTERFACE_TYPE(Printer2)
END_TYPE

THREAD_TYPE(Printer2)
	int i;
	INTERFACE_SLOT(Printer2);
END_TYPE

THREAD_BODY(Printer2)
	printf("Hi from printer thread: %d\n", this.i);
	++done;
END_BODY

int main()
{
	printf("Hi from main thread\n");
	run_thread(Printer2, .i=1);
	run_thread(Printer2, .i=2);
	printf("Bye from main thread\n");

	while (done < 2) { thread_yield(); }
}

// vim:ai:sw=4:ts=4:syntax=cpp
