// Sieve test
// gcc -Wall -O2 -lpthread filename.c

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "poly/scalar.h"
#include "poly/thread.h"
#include "poly/channel.h"

////////////////////////////////////////////////////////////////////////
// Tasks & Filters
////////////////////////////////////////////////////////////////////////

// generate 2,3,5,7,9...
THREAD_BODY (generate_candidates)
	Channel* input; // not used
	Channel* output;
THREAD_BEGIN (generate_candidates)
	assert(this.input == (Channel*)0);
	int n = 2;
	channel_send(this.output, n);
	for (n=3; true; n+=2)  { // forever odd numbers
		channel_send(this.output, n);
	}
THREAD_END

// filter multiples of prime
THREAD_BODY (filter_multiples)
	Channel* input;
	Channel* output;
	int prime;
THREAD_BEGIN (filter_multiples)
	Scalar s;
	int n;
	for (;;) {
		channel_receive(this.input, &s);
		n = cast(s, n);
		if (n%this.prime != 0) {
			channel_send_(this.output, s);
		}
	}
THREAD_END

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	enum { BOUND=100 };
	Scalar s;
	int p;

	Channel* ch = channel_alloc(1);
	warn(__func__);
	spawn_filter(0, ch, generate_candidates);

	for (int i = 0; i < BOUND; ++i) {
		channel_receive(ch, &s);
		p = cast(s, p);
		printf("%d\n", p);
		Channel* ch1 = channel_alloc(1);
	warn(__func__);
		spawn_filter(ch, ch1, filter_multiples, .prime=p);
		ch = ch1;
	}

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
