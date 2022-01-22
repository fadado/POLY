// Scalars test
// gcc -Wall -O2 -I. tests/scalar.c

#include <stdio.h>

#define DEBUG
#include "scalar.h"

int main(int argc, char** argv)
{
////////////////////////////////////////////////////////////////////////

	Scalar s1, s2;

	int i1, i2;
	s1 = Integer(-777);
	s2 = s1;
	i1 = cast(s1, int);
	assert(i1 == s1.integer);
	assert(i1 == s2.integer);
	i2 = cast(s1, signed short int);
	assert(i1 == i2);

	unsigned w1;
	s1 = Word(0x1000000U);
	s2 = s1;
	w1 = cast(s1, unsigned);
	assert(w1 == s1.word);
	assert(w1 == s2.word);

	double d1, d2;
	s1 = Real(123.456);
	s2 = s1;
	d1 = cast(s1, double);
	d2 = cast(s2, double);
	assert(d1 == d2);
	assert(d1 == s1.real);
	assert(d1 == s2.real);

////////////////////////////////////////////////////////////////////////

	int *p1;
	i1 = 123456;
	p1 = &i1;
	s1 = Pointer(p1);
	p1 = cast(s1, int*);
	assert(p1 == s1.pointer);

	int *p2;
	i1 = 123456;
	p2 = &i1;
	s1 = Pointer(p2);
	p2 = cast(s1, int*);
	assert(p2 == s1.pointer);
	s1 = Pointer(0);
	p2 = cast(s1, int*);
	assert(p2 == s1.pointer);

////////////////////////////////////////////////////////////////////////

	s1 = Integer(0);
	s1 = Word(0x0);
	s1 = Real(0e0);
	s1 = Pointer(00);

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
