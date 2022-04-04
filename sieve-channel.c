// Sieve with one thread for each prime
// gcc -Wall -O2 -lpthread sieve.c

#include <stdio.h>
#include <stdlib.h>

// comment next line to disable assertions
#define DEBUG
#include "poly/scalar.h"
#include "poly/thread.h"
#include "poly/task.h"
#include "poly/passing/channel.h"

////////////////////////////////////////////////////////////////////////
// Generate 2,3,5,7,9...
////////////////////////////////////////////////////////////////////////

TASK_TYPE (GenerateCandidates, static)
	Channel* input;
	Channel* output;
END_TYPE

TASK_BODY (GenerateCandidates)
	assert(this.input == (Channel*)0);

	int n = 2;
	channel_send(this.output, (Signed)n);

	for (n=3; true; n+=2)  {
		channel_send(this.output, (Signed)n);
	}
END_BODY

////////////////////////////////////////////////////////////////////////
// Filter multiples of `this->prime`
////////////////////////////////////////////////////////////////////////

TASK_TYPE (FilterMultiples, static)
	Channel* input;
	Channel* output;
	int      prime;
END_TYPE

TASK_BODY (FilterMultiples)
	inline ALWAYS bool divides(int n) {
		return n%this.prime == 0;
	}
	for (;;) {
		Scalar s;
		channel_receive(this.input, &s);
		if (!divides(cast(s, int))) {
			channel_send(this.output, s);
		}
	}
END_BODY

////////////////////////////////////////////////////////////////////////
// Start generator and successive filters
////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[argc+1])
{
	int err = 0;

	enum { NPRIMES=100 };
	int n = (argc == 1) ? NPRIMES : atoi(argv[1]);
	if (n <= 0) n = NPRIMES; // ignore bad parameter

	Channel _chn_arena[n+1], *_chn_ptr=_chn_arena;
	inline Channel* alloc(void) { return _chn_ptr++; }

	enum { syncronous=0, asyncronous=1 };
	Channel* input = alloc();
	channel_init(input, asyncronous);
	err = filter(GenerateCandidates, (Channel*)0, input);
	assert(err == 0);

	for (int i=1; i <= n; ++i) {
		Scalar s;
		channel_receive(input, &s);
		int prime = cast(s, int);

		Channel* output = alloc();
		channel_init(output, asyncronous);
		err = filter(FilterMultiples, input, output, .prime=prime);
		assert(err == 0);

		printf("%4d%c", prime, (i%10==0 ? '\n' : ' '));

		input = output;
	}
	putchar('\n');

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
