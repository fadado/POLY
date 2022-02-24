// Spinner test
// gcc -Wall -O2 -lpthread filename.c

#include <stdio.h>

// comment next line to disable assertions
#define DEBUG

// define thread_id() for 10 threads max.
#define THREAD_ID_SIZE 10

#include "poly/scalar.h"
#include "poly/thread.h"
#include "poly/task.h"
#include "poly/sugar.h"

#include "poly/syncop.h"

////////////////////////////////////////////////////////////////////////
// Playing with different methods of sync.
////////////////////////////////////////////////////////////////////////

#if 0

static atomic_flag calculating = ATOMIC_FLAG_INIT;

#define WAIT(r)    ({ TAS(&(r), RELAXED); while (TAS(&(r), ACQ_REL)); })
#define SIGNAL(r)  CLEAR(&(r), RELEASE)

#elif 0

static atomic_bool calculating = false;

#define WAIT(r)    while (!LOAD(&(r), ACQUIRE))
#define SIGNAL(r)  STORE(&(r), true, RELEASE)

#else

static volatile bool calculating = false;

#define WAIT(r)    while (!(r))
#define SIGNAL(r)  ((r) = true)

#endif

////////////////////////////////////////////////////////////////////////
// Run forever painting the spinner
////////////////////////////////////////////////////////////////////////

THREAD_BODY  (spinner)
	int delay; // nanoseconds
THREAD_BEGIN (spinner)
	warn("ThreadID: %d", thread_id());
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
THREAD_END

////////////////////////////////////////////////////////////////////////
// Compute fib(n) in the background
////////////////////////////////////////////////////////////////////////

THREAD_BODY  (fibonacci)
	Task* future;  // this is a task: a thread with future!
	long  n;
THREAD_BEGIN (fibonacci)
	auto long slow_fib(long x) {
		if (x < 2) { return x; }
		return slow_fib(x-1) + slow_fib(x-2);
	}

	warn("ThreadID: %d", thread_id());

	SIGNAL (calculating);

	long result = slow_fib(this.n);
	// ...long time...
	task_set(this.future, (Unsigned)result); // what if error: return > 0 ???
THREAD_END

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
	Time s,ms,us,ns, t = now();
	int err = 0;
	enum { N=46, usDELAY=500}; // fib(46)=1836311903

	hide_cursor();

	warn("ThreadID: %d", thread_id());

	err += spawn_thread(spinner, .delay=us2ns(usDELAY));

	Task fib_N;
	err += spawn_task(&fib_N, fibonacci, .n=N);
	err += task_join(&fib_N);

	assert(err==0);

	long n = cast(task_get(&fib_N), long);
	assert(n == 1836311903ul);
	printf("\rFibonacci(%d) = %ld\n", N, n);
	n = cast(task_get(&fib_N), long);
	printf("\rFibonacci(%d) = %ld\n", N, n);

	ns = now()-t;
	s = ns2s(ns);
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
