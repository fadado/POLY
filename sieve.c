// Sieve with one thread for each prime
// gcc -Wall -O2 -lpthread sieve.c

#include <stdio.h>
#include <stdlib.h>

// uncomment next line to enable assertions
//#define DEBUG
#include "poly/scalar.h"
#include "poly/thread.h"
#include "poly/channel.h"

// generate 2,3,5,7,9...
THREAD_BODY (generate_candidates)
	Channel* input; // not used
	Channel* output;
THREAD_BEGIN (generate_candidates)
	assert(this.input == (Channel*)0);
	int n = 2;
	channel_send(this.output, (Integer)n);
	for (n=3; true; n+=2)  { // forever odd numbers
		channel_send(this.output, (Integer)n);
	}
THREAD_END

// filter multiples of prime
THREAD_BODY (filter_multiples)
	Channel* input;
	Channel* output;
	int prime;
THREAD_BEGIN (filter_multiples)
	inline bool divides(int n, int d) { return n%d == 0; }
	Scalar s;
	for (;;) {
		channel_receive(this.input, &s);
		int n = cast(s, int);
		if (!divides(n, this.prime)) {
			channel_send(this.output, s);
		}
	}
THREAD_END

int main(int argc, char** argv)
{
	enum { NPRIMES=100 };
	int n = (argc == 1) ? NPRIMES : atoi(argv[1]);
	if (n <= 0) n = NPRIMES;

	Channel* input = channel_alloc(1);
	spawn_filter((Channel*)0, input, generate_candidates);

	for (int i = 0; i < n; ++i) {
		Scalar s;
		channel_receive(input, &s);
		int prime = cast(s, int);
		printf("%d ", prime);
		Channel* output = channel_alloc(1);
		spawn_filter(input, output, filter_multiples, .prime=prime);
		input = output;
	}
	putchar('\n');

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
