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

int main(void)
{
	int err;

	catch (run_thread(Hello));

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
