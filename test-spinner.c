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

typedef struct Future {
	Channel channel;
	Scalar  result;
	short   solved; // bool
	short   status;
} Future;

static ALWAYS inline int ftr_run(Future* future, int(*root)(void*), void* argument)
{
	future->solved = future->status = 0;
	int err = chn_init(&future->channel, 0);
	if (err == STATUS_SUCCESS) {
		err = tsk_run(root, (void*[2]){future, argument});
	}
	return err;
}

static ALWAYS inline int ftr_get(Future* future)
{
	if (!future->solved) {
		future->status = chn_receive(&future->channel, &future->result);
		future->solved = 1;
		chn_destroy(&future->channel);
	}
	return future->status;
}

#define ftr_set(F,S)    chn_send(&(F)->channel,(S))
#define ftr_resolved(F) (F)->solved
#define ftr_pending(F)  !(F)->solved
#define ftr_result(F,E) cast((F)->result,E)

#define async(F,R,...)  ftr_run(F, R, &(struct R){__VA_ARGS__})

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
	int err;
	enum { N=46, DELAY=500000}; // fib(46)=1836311903

	hide_cursor();

	err = run(task_spinner, .delay=DELAY);
	assert(err == STATUS_SUCCESS);

	Future future;
	err = async(&future, promise_fib, .n=N); assert(err == STATUS_SUCCESS);
	err = ftr_get(&future);                  assert(err == STATUS_SUCCESS);
	long n = ftr_result(&future, 0L);

	show_cursor();

	printf("\rFibonacci(%d) = %ld\n", N, n);
	err = ftr_get(&future);                  assert(err == STATUS_SUCCESS);
	n = ftr_result(&future, 0L);
	printf("\rFibonacci(%d) = %ld\n", N, n);
	return 0;
}


// vim:ai:sw=4:ts=4:syntax=cpp
