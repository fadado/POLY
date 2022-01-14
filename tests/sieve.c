// Sieve test
// gcc -Wall -O2 -I. -lpthread tests/filename.c

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "scalar.h"
#include "task.h"
#include "channel.h"

////////////////////////////////////////////////////////////////////////
// Tasks
////////////////////////////////////////////////////////////////////////

// generate 2,3,5,7,9...
TASK_BODY (generate_candidates)
	Channel* input; // not used
	Channel* output;
TASK_BEGIN (generate_candidates)
	assert(this->input == (Channel*)0);
	int n = 2;
	chn_send(this->output, n);
	for (n=3; true; n+=2)  { // forever odd numbers
		chn_send(this->output, n);
	}
TASK_END (generate_candidates)

// filter multiples of prime
TASK_BODY (filter_multiples)
	Channel* input;
	Channel* output;
	int prime;
TASK_BEGIN (filter_multiples)
	Scalar s;
	int n;
	for (;;) {
		chn_receive(this->input, &s);
		n = cast(s, n);
		if (n%this->prime != 0) {
			chn_send_(this->output, s);
		}
	}
TASK_END (filter_multiples)

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	enum { BOUND=100 };
	Scalar s;
	int p;

	Channel* ch = chn_alloc(1);
	warn(__func__);
	spawn_filter(0, ch, generate_candidates);

	for (int i = 0; i < BOUND; ++i) {
		chn_receive(ch, &s);
		p = cast(s, p);
		printf("%d\n", p);
		Channel* ch1 = chn_alloc(1);
	warn(__func__);
		spawn_filter(ch, ch1, filter_multiples, .prime=p);
		ch = ch1;
	}

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
