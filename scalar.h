/*
 * 64 bit scalars
 *
 */
#ifndef SCALAR_H
#define SCALAR_H

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

////////////////////////////////////////////////////////////////////////
// Type Scalar and 64 bit members
////////////////////////////////////////////////////////////////////////

typedef signed long long   Integer; // like int64_t; assume LP64 or LLP64
typedef unsigned long long Word;    // like uint64_t; assume LP64 or LLP64
typedef double             Real;    // the standard sets it
typedef void*              Pointer; // assume LP64 or LLP64

static_assert(sizeof(Real) == sizeof(Integer));
static_assert(sizeof(Real) == sizeof(Word));
static_assert(sizeof(Real) == sizeof(Pointer));
static_assert(sizeof(Real) == 8);

typedef union Scalar {
	Integer integer;
	Word    word;
	Real    real;
	Pointer pointer;
} Scalar
__attribute__((__transparent_union__));

static_assert(sizeof(Scalar) == 8);

// Cast from any scalar value to an Scalar
#define coerce(EXPRESSION) _Generic((EXPRESSION),\
	Scalar: (EXPRESSION),\
	_Bool: (Word)(EXPRESSION),\
	char: (Word)(EXPRESSION),\
	signed char: (Integer)(EXPRESSION),\
	unsigned char: (Word)(EXPRESSION),\
	signed short int: (Integer)(EXPRESSION),\
	unsigned short int: (Word)(EXPRESSION),\
	signed int: (Integer)(EXPRESSION),\
	unsigned int: (Word)(EXPRESSION),\
	signed long int: (Integer)(EXPRESSION),\
	unsigned long int: (Word)(EXPRESSION),\
	signed long long int: (Integer)(EXPRESSION),\
	unsigned long long int: (Word)(EXPRESSION),\
	float: (Real)(EXPRESSION),\
	double: (Real)(EXPRESSION),\
	long double: (Real)(EXPRESSION),\
	default: (Pointer)(Word)(EXPRESSION))

// Cast an Scalar to the same type as the example expression type
#define cast(SCALAR,EXAMPLE) _Generic((EXAMPLE),\
	_Bool: (_Bool)(SCALAR).word,\
	char: (char)(SCALAR).word,\
	signed char: (signed char)(SCALAR).integer,\
	unsigned char: (unsigned char)(SCALAR).word,\
	signed short int: (signed short int)(SCALAR).integer,\
	unsigned short int: (unsigned short int)(SCALAR).word,\
	signed int: (signed int)(SCALAR).integer,\
	unsigned int: (unsigned int)(SCALAR).word,\
	signed long int: (signed long int)(SCALAR).integer,\
	unsigned long int: (unsigned long int)(SCALAR).word,\
	signed long long int: (signed long long int)(SCALAR).integer,\
	unsigned long long int: (unsigned long long int)(SCALAR).word,\
	float: (float)(SCALAR).real,\
	double: (double)(SCALAR).real,\
	long double: (long double)(SCALAR).real,\
	default: (void*)(SCALAR).pointer)

#endif // SCALAR_H

// vim:ai:sw=4:ts=4:syntax=cpp
