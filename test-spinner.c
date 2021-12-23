// Spinner test

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "POLY.h"
#include "task.h"

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

static int slow_fib(int x)
{
	if (x < 2) {
		return x;
	}
	return slow_fib(x-1) + slow_fib(x-2);
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

static int task_spinner(void* args)
{
	int delay = *(int*)args;

	const char s[] = "-\\|/-";

	ALWAYS inline void spin(int i) {
		putchar('\r'); putchar(' '); putchar(s[i]);
		tsk_sleep(delay);
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
	enum { N=46 };
	int delay = 500000;

	hide_cursor();

	tsk_run(task_spinner, &delay);

	int f = slow_fib(N);

	show_cursor();

	printf("\rFibonacci(%d) = %d\n", N, f);
	return 0;
}


// vim:ai:sw=4:ts=4:syntax=cpp
