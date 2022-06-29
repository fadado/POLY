// Sieve with one thread for each prime
// gcc -Wall -O2 -lpthread sieve.c

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include <stdio.h>
#include <stdlib.h>

// comment next line to disable assertions
#define DEBUG
#include "poly/thread.h"
#include "poly/scalar.h"
#include "poly/passing/port.h"

////////////////////////////////////////////////////////////////////////
// Generate 2,3,5,7,9...
////////////////////////////////////////////////////////////////////////

struct Candidates {
	THREAD_TYPE
	Port* input;
	Port* output;
};

int Candidates(void* data)
{
	THREAD_BODY (Candidates, data)

	assert(this.input == NULL);

	int n = 2;
	port_send(this.output, (Signed)n);

	for (n=3; true; n+=2)  {
		port_send(this.output, (Signed)n);
	}

	END_BODY
}

////////////////////////////////////////////////////////////////////////
// Filter multiples of `this->prime`
////////////////////////////////////////////////////////////////////////

struct Sieve {
	THREAD_TYPE
	Port* input;
	Port* output;
	int prime;
};

int Sieve(void* data)
{
	THREAD_BODY (Sieve, data)

	inline ALWAYS bool divides(int n) {
		return n%this.prime == 0;
	}
	for (;;) {
		Scalar s;
		port_receive(this.input, &s);
		if (!divides(cast(s, int))) {
			port_send(this.output, s);
		}
	}

	END_BODY
}

////////////////////////////////////////////////////////////////////////
// Start generator and successive filters
////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[argc+1])
{
	enum { NPRIMES=100 };
	int n = (argc == 1) ? NPRIMES : atoi(argv[1]);
	if (n <= 0) n = NPRIMES; // ignore bad parameter

	Port _port_arena[n+1], *_port_ptr=_port_arena;
	inline Port* alloc(void) { return _port_ptr++; }

	Port* input = alloc();
	port_init(input);
	run_filter(Candidates, NULL, input);

	for (int i=1; i <= n; ++i) {
		Scalar s;
		port_receive(input, &s);
		int prime = cast(s, int);

		Port* output = alloc();
		port_init(output);
		run_filter(Sieve, input, output, .prime=prime);

		printf("%4d%c", prime, (i%10==0 ? '\n' : ' '));

		input = output;
	}
	putchar('\n');

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
