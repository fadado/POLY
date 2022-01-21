// Spinner test
// gcc -Wall -O2 -I. -lpthread tests/filename.c

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "scalar.h"
#include "task.h"
#include "future.h"

TASK_SPEC (spinner, static)
TASK_SPEC (fibonacci, static)

DEFINE_TASK_ID (10) // max 10 tasks

////////////////////////////////////////////////////////////////////////
// Tasks
////////////////////////////////////////////////////////////////////////

// run forever painting the spinner
TASK_BODY  (spinner)
	int delay; // nanoseconds
TASK_BEGIN (spinner)
	warn("TaskID: %d", task_id());
	const char s[] = "-\\|/-";
	inline void spin(int i) {
		putchar('\r'); putchar(' '); putchar(s[i]);
		task_sleep(this->delay);
	}

	spin(0);
	for (;;)  { // forever
		for (int i = 0; s[i] != '\0'; ++i) {
			spin(i);
		}
	}
TASK_END

// compute fib(n) in the background
TASK_BODY  (fibonacci)
	Future* future;  // this is a promise: a task with future!
	long    n;
TASK_BEGIN (fibonacci)
	warn("TaskID: %d", task_id());
	auto long slow_fib(long x) {
		if (x < 2) { return x; }
		return slow_fib(x-1) + slow_fib(x-2);
	}

	long result = slow_fib(this->n);
	// ...long time...
	future_set(this->future, coerce(result)); // what if error: return > 0 ???
TASK_END

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
	unsigned long long s,ms,us,ns, t = now();
	int err = 0;
	enum { N=46, usDELAY=500}; // fib(46)=1836311903

	hide_cursor();

	warn("TaskID: %d", task_id());

	err += spawn_task(spinner, .delay=us2ns(usDELAY));

	Future future;
	err += spawn_future(&future, fibonacci, .n=N);
	err += future_join(&future);

	assert(err==0);

	long n = cast(future_get(&future), 0L);
	assert(n == 1836311903ul);
	printf("\rFibonacci(%d) = %ld\n", N, n);
	n = cast(future_get(&future), 0L);
	printf("\rFibonacci(%d) = %ld\n", N, n);

	ns = now()-t;
	s = ns2s(ns);
	ms = ns2ms(ns - s2ns(s));
	us = ns2us(ns - s2ns(s) - ms2ns(ms));
	ns = (ns - s2ns(s) - ms2ns(ms) - us2ns(us));
	printf("s: %lld; ms: %lld; un: %lld; ns: %lld\n", s, ms, us, ns);

	show_cursor();

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
