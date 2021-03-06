// Scalars test
// gcc -Wall -O2 filename.c
#pragma GCC diagnostic ignored "-Wunused-function"

#include <stdio.h>

#define DEBUG
#include "poly/scalar.h"

#include "poly/passing/interface.h"

typedef struct {
    Entry a;
    Entry b;
    Entry c;
} ITest;

static_assert(sizeof(ITest) == sizeof(Entry)*3);

int main(int argc, char* argv[argc+1])
{
////////////////////////////////////////////////////////////////////////

    ITest iface;
    assert(ENTRIES(iface) == 3);
    interface_init(ENTRIES(ITest), &iface);
    interface_destroy(ENTRIES(ITest), &iface);

////////////////////////////////////////////////////////////////////////

	Scalar s1, s2 = {0};

	int i1, i2;
	s1 = Signed(-777LL);
	s2 = s1;
	i1 = cast(s1, int);
	assert(i1 == s1.i);
	assert(i1 == s2.i);
	i2 = cast(s1, signed short int);
	assert(i1 == -777);
	assert(i1 == i2);

	unsigned w1;
	s1 = Unsigned(0x1000000ULL);
	s2 = s1;
	w1 = cast(s1, unsigned);
	assert(w1 == 0x1000000U);
	assert(w1 == s1.u);
	assert(w1 == s2.u);

	double d1, d2;
	s1 = Double(123.456);
	s2 = s1;
	d1 = cast(s1, double);
	d2 = cast(s2, double);
	assert(d1 == 123.456);
	assert(d1 == d2);
	assert(d1 == s1.d);
	assert(d1 == s2.d);

////////////////////////////////////////////////////////////////////////

	int *p1;
	i1 = 123456;
	p1 = &i1;
	s1 = Pointer(p1);
	p1 = cast(s1, int*);
	assert(p1 == s1.p);

	int *p2;
	i1 = 123456;
	p2 = &i1;
	s1 = Pointer(p2);
	p2 = cast(s1, int*);
	assert(p2 == s1.p);
	s1 = Pointer(00);
	p2 = cast(s1, int*);
	assert(p2 == s1.p);


	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
