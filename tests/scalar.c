// Scalars test
// gcc -Wall -O2 -I. tests/scalar.c

#include <stdio.h>

// uncomment next line to enable assertions
#define DEBUG
#include "scalar.h"

int main(int argc, char** argv)
{

	inline ALWAYS void set_(Scalar* p, Scalar s) {
		*p = s;
	}
#	define set(P,S) set_((P), coerce(S))

	Scalar s1, s2; int i1;
	s1 = coerce(-777);			// assign literal int to Scalar
	s2 = s1;					// assign (copy) Scalar to Scalar
	i1 = cast(s1, i1);			// restore integer from Scalar
	assert(i1 == s1.integer);
	assert(i1 == s2.integer);

	unsigned w1;
	s1 = coerce(0x1000000U);	// assign literal unsigned to Scalar
	s2 = s1;					// assign (copy) Scalar to Scalar
	w1 = cast(s1, w1);			// restore word from Scalar
	assert(w1 == s1.word);
	assert(w1 == s2.word);

	double d1;
	s1 = coerce(123.456);		// assign literal double to Scalar
	s2 = s1;					// assign (copy) Scalar to Scalar
	d1 = cast(s1, d1);			// restore integer from Scalar
	assert(d1 == s1.real);
	assert(d1 == s2.real);


	//int* p1;
	////s1.pointer = &s1;
	//s1 = coerce(&i1);    // assign pointer to Scalar
	//s2 = s1;					// assign (copy) Scalar to Scalar
	//p1 = cast(s1, p1);			// restore integer from Scalar
	//assert(p1 == s1.pointer);
	//assert(p1 == s2.pointer);


	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
