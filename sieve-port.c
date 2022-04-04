// Sieve with one thread for each prime
// gcc -Wall -O2 -lpthread sieve.c

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include <stdio.h>
#include <stdlib.h>

// comment next line to disable assertions
#define DEBUG
#include "poly/scalar.h"
#include "poly/thread.h"
#include "poly/task.h"
#include "poly/passing/port.h"

////////////////////////////////////////////////////////////////////////
// Generate 2,3,5,7,9...
////////////////////////////////////////////////////////////////////////

TASK_TYPE (GenerateCandidates, static)
	Port* input;
	Port* output;
END_TYPE

TASK_BODY (GenerateCandidates)
	assert(this.input == (Port*)0);

	int n = 2;
	port_send(this.output, (Signed)n);

	for (n=3; true; n+=2)  {
		port_send(this.output, (Signed)n);
	}
END_BODY

////////////////////////////////////////////////////////////////////////
// Filter multiples of `this->prime`
////////////////////////////////////////////////////////////////////////

TASK_TYPE (FilterMultiples, static)
	Port* input;
	Port* output;
	int prime;
END_TYPE

TASK_BODY (FilterMultiples)
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

////////////////////////////////////////////////////////////////////////
// Start generator and successive filters
////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[argc+1])
{
	int err = 0;

	enum { NPRIMES=100 };
	int n = (argc == 1) ? NPRIMES : atoi(argv[1]);
	if (n <= 0) n = NPRIMES; // ignore bad parameter

	Port _port_arena[n+1], *_port_ptr=_port_arena;
	inline Port* alloc(void) { return _port_ptr++; }

	Port* input = alloc();
	port_init(input);
	err = filter(GenerateCandidates, (Port*)0, input);
	assert(err == 0);

	for (int i=1; i <= n; ++i) {
		Scalar s;
		port_receive(input, &s);
		int prime = cast(s, int);

		Port* output = alloc();
		port_init(output);
		err = filter(FilterMultiples, input, output, .prime=prime);
		assert(err == 0);

		printf("%4d%c", prime, (i%10==0 ? '\n' : ' '));

		input = output;
	}
	putchar('\n');

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
