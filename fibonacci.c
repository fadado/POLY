// Spinner test
// gcc -Wall -O2 -lpthread filename.c

#include <stdio.h>

// comment next line to disable assertions
#define DEBUG

#include "poly/thread.h"
#include "poly/task.h"
#include "poly/atomics.h"
#include "poly/scalar.h"
#include "poly/pass/future.h"


////////////////////////////////////////////////////////////////////////
// Playing with different methods of sync.
////////////////////////////////////////////////////////////////////////

#if 0

static atomic_flag calculating = ATOMIC_FLAG_INIT;

#define WAIT(R)    ({ TAS(&(R), RELAXED); while (TAS(&(R), ACQ_REL)); })
#define SIGNAL(R)  CLEAR(&(R), RELEASE)

#elif 0

static atomic_bool calculating = false;

#define WAIT(R)    while (!LOAD(&(R), ACQUIRE))
#define SIGNAL(R)  STORE(&(R), true, RELEASE)

#else

static volatile bool calculating = false;

#define WAIT(R)    while (!(R))
#define SIGNAL(R)  ((R) = true)

#endif

////////////////////////////////////////////////////////////////////////
// Run forever painting the spinner
////////////////////////////////////////////////////////////////////////

TASK_TYPE (Spinner, static)
	int delay; // nanoseconds
END_TYPE

TASK_BODY (Spinner)
	warn("TaskID: %d", TASK_ID);
	const char s[] = "-\\|/-";
	inline void spin(int i) {
		putchar('\r'); putchar(' '); putchar(s[i]);
		thread_sleep(this.delay);
	}

	WAIT (calculating);

	spin(0);
	for (;;)  {
		for (int i = 0; s[i] != '\0'; ++i) {
			spin(i);
		}
	}
END_BODY

////////////////////////////////////////////////////////////////////////
// Compute fib(n) in the background
////////////////////////////////////////////////////////////////////////

TASK_TYPE (Fibonacci, static)
	Future* future;
	long    n;
END_TYPE

TASK_BODY (Fibonacci)
	auto long slow_fib(long x) {
		if (x < 2) { return x; }
		return slow_fib(x-1) + slow_fib(x-2);
	}

	warn("TaskID: %d", TASK_ID);

	SIGNAL (calculating);

	long result = slow_fib(this.n);
	// ...long time...
	future_send(this.future, (Unsigned)result); // what if error: return > 0 ???
END_BODY

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

#define CSI "\x1b["
#define SHOW CSI "?25h"
#define HIDE CSI "?25l"

#define hide_cursor() printf(HIDE)
#define show_cursor() printf(SHOW)

int main(int argc, char* argv[argc+1])
{
	int err = 0;

	enum { N=46, usDELAY=500}; // fib(46)=1836311903
	Clock s,ms,us,ns, t = now();

	hide_cursor();

	warn("TaskID: %d", TASK_ID);

	err += RUN_task(Spinner, .delay=us2ns(usDELAY));

	Future inbox;
	err += RUN_promise(Fibonacci, &inbox, .n=N);
	// ... time
	err += future_join(&inbox);

	assert(err == 0);

	long n = cast(future_receive(&inbox), long);
	assert(n == 1836311903ul);
	printf("\rFibonacci(%d) = %ld\n", N, n);
	n = cast(future_receive(&inbox), long);
	printf("\rFibonacci(%d) = %ld\n", N, n);

	ns = now()-t;
	s  = ns2s(ns);
	ms = ns2ms(ns - s2ns(s));
	us = ns2us(ns - s2ns(s) - ms2ns(ms));
	ns = (ns - s2ns(s) - ms2ns(ms) - us2ns(us));
	printf("s: %lld; ms: %lld; un: %lld; ns: %lld\n", s, ms, us, ns);

	show_cursor();

//
	atomic_int shared = 7;
	MUL(&shared, 7);
	assert(shared == 7*7);

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
