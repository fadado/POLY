// Spinner test
// gcc -Wall -O2 -lpthread filename.c

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "poly/scalar.h"
#include "poly/thread.h"
#include "poly/task.h"

THREAD_SPEC (spinner, static)
THREAD_SPEC (fibonacci, static)

DEFINE_THREAD_ID (10) // max 10 threads

////////////////////////////////////////////////////////////////////////
// Tasks
////////////////////////////////////////////////////////////////////////

// run forever painting the spinner
THREAD_BODY  (spinner)
	int delay; // nanoseconds
THREAD_BEGIN (spinner)
	warn("ThreadID: %d", thread_id());
	const char s[] = "-\\|/-";
	inline void spin(int i) {
		putchar('\r'); putchar(' '); putchar(s[i]);
		thread_sleep(this.delay);
	}

	spin(0);
	for (;;)  { // forever
		for (int i = 0; s[i] != '\0'; ++i) {
			spin(i);
		}
	}
THREAD_END

// compute fib(n) in the background
THREAD_BODY  (fibonacci)
	Future* future;  // this is a task: a thread with future!
	long    n;
THREAD_BEGIN (fibonacci)
	warn("ThreadID: %d", thread_id());
	auto long slow_fib(long x) {
		if (x < 2) { return x; }
		return slow_fib(x-1) + slow_fib(x-2);
	}

	long result = slow_fib(this.n);
	// ...long time...
	future_set(this.future, (Integer)result); // what if error: return > 0 ???
THREAD_END

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
	Time s,ms,us,ns, t = now();
	int err = 0;
	enum { N=46, usDELAY=500}; // fib(46)=1836311903

	hide_cursor();

	warn("ThreadID: %d", thread_id());

	err += spawn_thread(spinner, .delay=us2ns(usDELAY));

	Future future;
	err += spawn_task(&future, fibonacci, .n=N);
	err += task_join(&future);

	assert(err==0);

	long n = cast(future_get(&future), long);
	assert(n == 1836311903ul);
	printf("\rFibonacci(%d) = %ld\n", N, n);
	n = cast(future_get(&future), long);
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
