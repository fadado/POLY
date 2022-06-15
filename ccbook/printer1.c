/* printer1.c */
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <stdio.h>
#include "poly/thread.h"
#include "poly/passing/entry.h"
#include "poly/passing/interface.h"

static atomic(bool) done;

INTERFACE_TYPE(Printer1)
END_TYPE

THREAD_TYPE(Printer1)
	INTERFACE_SLOT(Printer1);
END_TYPE

THREAD_BODY(Printer1)
	printf("Hi from printer thread\n");
	done = true;
END_BODY

int main()
{
	int err;

	printf("Hi from main thread\n");
	catch (run_thread(Printer1));
	printf("Bye from main thread\n");

	while (!done) { thread_yield(); }

	return 0;
onerror:
	char* msg;
	switch (err) {
		case STATUS_BUSY: msg="BUSY"; break;
		case STATUS_ERROR: msg="ERROR"; break;
		case STATUS_NOMEM: msg="NOMEM"; break;
		case STATUS_TIMEDOUT: msg="TIMEDOUT"; break;
		default: msg="unknown error code"; break;
	}
	error(1, 0, "%s (%d)", msg, err);
}

// vim:ai:sw=4:ts=4:syntax=cpp
