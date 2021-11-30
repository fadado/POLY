// Semaphore test

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

// uncomment next line to enable assertions
#define DEBUG
#include "failure.h"
#include "semaphore.h"

/*
 *
 */
int main(int argc, char* argv[])
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) goto onerror

	Semaphore semaphore;
	catch (sem_init(&semaphore, 1));

	sem_P(&semaphore);
	sem_V(&semaphore);

	sem_destroy(&semaphore);

	return EXIT_SUCCESS;
onerror:
#	undef catch
	static const char* ename[] = {
		[thrd_success] = "thrd_success",
		[thrd_nomem] = "thrd_nomem",
		[thrd_busy] = "thrd_busy",
		[thrd_error] = "thrd_error",
		[thrd_timedout] = "thrd_timedout",
	};

	assert(err != thrd_success);
	switch (err) {
		case thrd_nomem:
			error(EXIT_FAILURE, ENOMEM, "%s", ename[err]);
		case thrd_busy:
			error(EXIT_FAILURE, EBUSY, "%s", ename[err]);
		case thrd_error:
			error(EXIT_FAILURE, ECANCELED, "%s", ename[err]);
		case thrd_timedout:
			error(EXIT_FAILURE, ETIMEDOUT, "%s", ename[err]);
		default: assert(internal_error);
	}
	return EXIT_FAILURE;
}

// vim:ai:sw=4:ts=4:syntax=cpp
