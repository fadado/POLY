// Spinner test

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "POLY.h"
#include "scalar.h"
#include "task.h"
#include "channel.h"

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

#define run(T,...) tsk_run(T, &(struct T){__VA_ARGS__})

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

#define UsingPromises 1

#if UsingPromises

typedef struct Future {
	Channel channel;
	Scalar  result;
	int     solved; // bool
	int     status;
} Future;

struct void_pair {
	void* fut;
	void* arg;
};

#define async(F,U,...)\
	tsk_run(F, &(struct void_pair){U,&(struct F){__VA_ARGS__}})

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

struct promise_fib {
	long n;
};

static int promise_fib(void* arg)
{
	struct Future* future  = ((struct void_pair*)arg)->fut;
	struct promise_fib* my = ((struct void_pair*)arg)->arg;

	long fib = slow_fib(my->n);

	chn_send(&future->channel, fib);

	return 0; // tsk_exit(0);
}

#endif

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
	enum { N=46, DELAY=500000}; // fib(46)=1836311903

	hide_cursor();

	run(task_spinner, .delay=DELAY);

#if UsingPromises
	Future future;
	chn_init(&future.channel, 1);

	async(promise_fib, &future, .n=N);
	chn_receive(&future.channel, &future.result);
	chn_destroy(&future.channel);

	long n = cast(future.result, 0L);
#else
	long n = slow_fib(N);
#endif

	show_cursor();

	printf("\rFibonacci(%d) = %ld\n", N, n);
	return 0;
}


// vim:ai:sw=4:ts=4:syntax=cpp
