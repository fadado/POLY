// Spinner test

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "scalar.h"
#include "task.h"
#include "future.h"

TASK_SPEC(spinner, static)
TASK_SPEC(fibonacci, static)

////////////////////////////////////////////////////////////////////////
// Tasks
////////////////////////////////////////////////////////////////////////

// run forever painting the spinner
TASK_BODY(spinner)
	int delay; // nanoseconds
TASK_BEGIN(spinner)
	const char s[] = "-\\|/-";

	inline void spin(int i) {
		putchar('\r'); putchar(' '); putchar(s[i]);
		tsk_sleep(self->delay);
	}

	spin(0);
	for (;;)  { // forever
		for (int i = 0; s[i] != '\0'; ++i) {
			spin(i);
		}
	}
TASK_END(spinner)

// compute fib(n) in the background
TASK_BODY(fibonacci)
	long n;
PROMISE_BEGIN(fibonacci)
	auto long slow_fib(long x) {
		if (x < 2) { return x; }
		return slow_fib(x-1) + slow_fib(x-2);
	}

	long result = slow_fib(self->n);
	// ...long time...
	ftr_set(future, result); // what if return > 0 ???
TASK_END(fibonacci)

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

#define CSI "\x1b["
#define SHOW CSI "?25h"
#define HIDE CSI "?25l"

#define hide_cursor() printf(HIDE)
#define show_cursor() printf(SHOW)

int main(int argc, char** argv)
{
	int err = 0;
	enum { N=46, DELAY=us2ns(500)}; // fib(46)=1836311903

	hide_cursor();

	err += tsk_spawn(spinner, .delay=DELAY);

	Future future;
	err += ftr_spawn(&future, fibonacci, .n=N);
	err += ftr_wait(&future);

	assert(err==0);

	long n = cast(ftr_get(&future), 0L);
	assert(n == 1836311903ul);
	printf("\rFibonacci(%d) = %ld\n", N, n);
	n = cast(ftr_get(&future), 0L);
	printf("\rFibonacci(%d) = %ld\n", N, n);

	show_cursor();

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
