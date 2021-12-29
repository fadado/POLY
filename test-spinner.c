// Spinner test

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "POLY.h"
#include "scalar.h"
#include "task.h"
#include "future.h"

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

struct task_spinner {
	int delay;
};
static int task_spinner(void* arg)
{
	struct task_spinner* my = arg;

	const char s[] = "-\\|/-";

	ALWAYS inline void spin(int i) {
		putchar('\r'); putchar(' '); putchar(s[i]);
		tsk_sleep(my->delay);
	}

	spin(0);
	while (1) {
		for (int i = 0; s[i] != '\0'; ++i) {
			spin(i);
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

static long slow_fib(long x)
{
	if (x < 2) {
		return x;
	}
	return slow_fib(x-1) + slow_fib(x-2);
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

struct promise_fib {
	long n;
};
static int promise_fib(void* arg)
{
	struct Future* future  = ((void**)arg)[0];
	struct promise_fib* my = ((void**)arg)[1];

	long fib = slow_fib(my->n);

	ftr_set(future, fib);

	return 0; // tsk_exit(0);
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

#define CSI "\x1b["
#define SHOW CSI "?25h"
#define HIDE CSI "?25l"

#define hide_cursor() printf(HIDE)
#define show_cursor() printf(SHOW)

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	int err = 0;
	enum { N=46, DELAY=500000}; // fib(46)=1836311903

	hide_cursor();

	err += tsk_spawn(task_spinner, .delay=DELAY);

	Future future;
	err += ftr_spawn(&future, promise_fib, .n=N);
	err += ftr_wait(&future);

	assert(err==0);

	long n = cast(ftr_get(&future), 0L);
	printf("\rFibonacci(%d) = %ld\n", N, n);
	n = cast(ftr_get(&future), 0L);
	printf("\rFibonacci(%d) = %ld\n", N, n);

	show_cursor();

	return 0;
}


// vim:ai:sw=4:ts=4:syntax=cpp
