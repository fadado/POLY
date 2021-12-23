// Spinner test

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "POLY.h"

static int slow_fib(int x)
{
	if (x < 2) {
		return x;
	}
	return slow_fib(x-1) + slow_fib(x-2);
}

static int spinner(void* args)
{
	int delay = *(int*)args;

	char s[5] = "-\\|/";
	while (1) {
		for (int i = 0; s[i] != '\0'; ++i) {
			printf("\r%c", s[i]);
			thrd_sleep(&(struct timespec){.tv_nsec=delay*10}, 0);
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	enum { N=45 };
	int delay = 1000000;

	thrd_t t;
	thrd_create(&t, &spinner, &delay);
	thrd_detach(t);

	int f = slow_fib(N);
	printf("\rFibonacci(%d) = %d\n", N, f);
	return 0;
}


// vim:ai:sw=4:ts=4:syntax=cpp
