// Scalars test
// gcc -Wall -O2 filename.c

#include <stdio.h>

#define DEBUG
#include "poly/scalar.h"

union Scalar scalar(union Scalar s) { return s; }

int main(int argc, char** argv)
{
////////////////////////////////////////////////////////////////////////

	Scalar s1, s2;

	int i1, i2;
	s1 = Scalar(-777);
	s1 = Scalar(-777);
	s2 = s1;
	i1 = cast(s1, int);
	assert(i1 == s1.i);
	assert(i1 == s2.i);
	i2 = cast(s1, signed short int);
	assert(i1 == i2);

	unsigned w1;
	s1 = Scalar(0x1000000U);
	s2 = s1;
	w1 = cast(s1, unsigned);
	assert(w1 == s1.u);
	assert(w1 == s2.u);

	double d1, d2;
	//s1 = Scalar(123.456);
	s2 = s1;
	d1 = cast(s1, double);
	d2 = cast(s2, double);
	assert(d1 == d2);
	assert(d1 == s1.d);
	assert(d1 == s2.d);

////////////////////////////////////////////////////////////////////////

	int *p1;
	i1 = 123456;
	p1 = &i1;
	//s1 = Scalar(p1);
	s1 = Pointer(p1);
	p1 = cast(s1, int*);
	assert(p1 == s1.p);

	int *p2;
	i1 = 123456;
	p2 = &i1;
	s1 = Pointer(p2);
	p2 = cast(s1, int*);
	assert(p2 == s1.p);
	s1 = Pointer(0);
	p2 = cast(s1, int*);
	assert(p2 == s1.p);

////////////////////////////////////////////////////////////////////////

	s1 = Scalar(0);
	s1 = Scalar(0x0U);
	//s1 = Scalar((float)0e0);
	s1 = Pointer(0);

	Scalar s3 = { .i=-77 };
	Scalar s4 = { -77 };
	//Scalar s5 = 0;
	assert(cast(s3,int) == cast(s4,int));

	Scalar s5 = (Scalar){.d=(Double)7.7};
	Scalar s6 = (Scalar){.d=7.7};
	assert(cast(s5,double) == cast(s6,double));
	printf("%g\n", cast(s5,double));

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
