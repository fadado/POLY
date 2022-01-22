/*
 * 64 bit scalars
 *
 */
#ifndef SCALAR_H
#define SCALAR_H

#ifndef POLY_H
#include "POLY.h"
#endif

////////////////////////////////////////////////////////////////////////
// Scalar type and 64 bit scalar types
////////////////////////////////////////////////////////////////////////

typedef signed long long   Integer; // like int64_t; assume LP64 or LLP64
typedef unsigned long long Natural; // like uint64_t; assume LP64 or LLP64
typedef double             Real;    // the standard sets it
typedef void*              Pointer; // assume LP64 or LLP64

static_assert(sizeof(Real) == sizeof(Integer));
static_assert(sizeof(Real) == sizeof(Natural));
static_assert(sizeof(Real) == sizeof(Pointer));
static_assert(sizeof(Real) == 8);

typedef union TRANSPARENT Scalar {
	Integer integer;
	Natural word;
	Real    real;
	Pointer pointer;
} Scalar;

static_assert(sizeof(Scalar) == 8);

////////////////////////////////////////////////////////////////////////
// Cast to and from Scalar
////////////////////////////////////////////////////////////////////////

#ifdef TheCoerceMacroOrTheCompilerHaveBugs

// Cast from any native scalar EXPRESSION to an Scalar
#define coerce(EXPRESSION) (Scalar)_Generic((EXPRESSION),\
	_Bool: (Natural)(EXPRESSION),\
	char: (Natural)(EXPRESSION),\
	signed char: (Integer)(EXPRESSION),\
	unsigned char: (Natural)(EXPRESSION),\
	signed short int: (Integer)(EXPRESSION),\
	unsigned short int: (Natural)(EXPRESSION),\
	signed int: (Integer)(EXPRESSION),\
	unsigned int: (Natural)(EXPRESSION),\
	signed long int: (Integer)(EXPRESSION),\
	unsigned long int: (Natural)(EXPRESSION),\
	signed long long int: (Integer)(EXPRESSION),\
	unsigned long long int: (Natural)(EXPRESSION),\
	float: (Real)(EXPRESSION),\
	double: (Real)(EXPRESSION),\
	long double: (Real)(EXPRESSION))

//	this causes an error!!!
//	default: (Pointer)(Natural)(EXPRESSION)) ???

#endif

// alternative to coerce
#define Integer(x)    (Scalar)(Integer)(x)
#define Natural(x)    (Scalar)(Natural)(x)
#define Real(x)       (Scalar)(Real)(x)
#define Pointer(x)    (Scalar)(Pointer)(x)

// Cast an union SCALAR to the TYPE specified
#define cast(SCALAR,TYPE) _Generic(((TYPE){0}),\
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
	default: (SCALAR).pointer)

#endif // SCALAR_H

// vim:ai:sw=4:ts=4:syntax=cpp
