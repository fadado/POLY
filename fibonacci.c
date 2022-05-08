// Spinner test
// gcc -Wall -O2 -lpthread filename.c

#include <stdio.h>

// comment next line to disable assertions
#define DEBUG

#include "poly/thread.h"
#include "poly/task.h"
#include "poly/scalar.h"
#include "poly/pass/channel.h"

////////////////////////////////////////////////////////////////////////

#include "poly/atomics.h"

typedef atomic_uint_fast8_t Event;

static ALWAYS inline void
event_wait (Event* this)
{
	while (!LOAD(this, ACQUIRE)) {
		/*spin*/;
	}
}

static ALWAYS inline void
event_notify (Event* this)
{
	STORE(this, 1, RELEASE);
}

static ALWAYS inline void
event_clear (Event* this)
{
	STORE(this, 0, RELEASE);
}

////////////////////////////////////////////////////////////////////////
// Run forever painting the spinner
////////////////////////////////////////////////////////////////////////

static Event calculating = {0};

TASK_TYPE (Spinner, static)
	int delay; // nanoseconds
END_TYPE

TASK_BODY (Spinner)
	const char s[] = "-\\|/-";
	inline void spin(int i) {
		putchar('\r'); putchar(' '); putchar(s[i]);
		thread_sleep(this.delay);
	}

	warn("TaskID: %d", TASK_ID);
	event_wait(&calculating);

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
	Channel* future;
	long     n;
END_TYPE

TASK_BODY (Fibonacci)

	auto long slow_fib(long x) {
		if (x < 2) { return x; }
		return slow_fib(x-1) + slow_fib(x-2);
	}

	warn("TaskID: %d", TASK_ID);
	event_notify(&calculating);

	long result = slow_fib(this.n);
	// ...long time...
	channel_send(this.future, (Unsigned)result); // what if error: return > 0 ???
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

	Channel inbox;
	err += channel_init(&inbox, 1);
	err += RUN_promise(Fibonacci, &inbox, .n=N);
	//err+=RUN_task(Fibonacci, .future=&inbox, .n=N);

	assert(err == 0);

	Scalar r;
	err = channel_receive(&inbox, &r);
	assert(err == 0);
	long n = cast(r, long);
	assert(n == 1836311903ul);
	printf("\rFibonacci(%d) = %ld\n", N, n);

	ns = now()-t;
	s  = ns2s(ns);
	ms = ns2ms(ns - s2ns(s));
	us = ns2us(ns - s2ns(s) - ms2ns(ms));
	ns = (ns - s2ns(s) - ms2ns(ms) - us2ns(us));
	printf("s: %lld; ms: %lld; un: %lld; ns: %lld\n", s, ms, us, ns);

	show_cursor();

	channel_destroy(&inbox);

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
