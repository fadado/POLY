#ifndef SCALAR_H
#define SCALAR_H

#ifndef POLY_H
#include "POLY.h"
#endif

////////////////////////////////////////////////////////////////////////
// Scalar type and 64 bit scalar types
////////////////////////////////////////////////////////////////////////

typedef signed long long   Integer;
typedef unsigned long long Unsigned;
typedef double             Double;
typedef void*              Pointer;

static_assert(sizeof(Double) == sizeof(Integer));
static_assert(sizeof(Double) == sizeof(Unsigned));
static_assert(sizeof(Double) == sizeof(Pointer));
static_assert(sizeof(Double) == 8);

typedef union TRANSPARENT Scalar {
	Integer  i;
	Unsigned u;
	Double   d;
	Pointer  p;
} Scalar;

static_assert(sizeof(union Scalar) == 8);

////////////////////////////////////////////////////////////////////////
// Cast to and from Scalar
////////////////////////////////////////////////////////////////////////

// Scalar constructors
#define Pointer(x)    (Scalar){.p=(x)}

// Cast from any native scalar EXPRESSION to an Scalar
#if 0
#define Scalar(EXPRESSION) (union Scalar)_Generic((EXPRESSION),\
	_Bool: (Unsigned)(EXPRESSION),\
	char: (Unsigned)(EXPRESSION),\
	signed char: (Integer)(EXPRESSION),\
	unsigned char: (Unsigned)(EXPRESSION),\
	signed short int: (Integer)(EXPRESSION),\
	unsigned short int: (Unsigned)(EXPRESSION),\
	signed int: (Integer)(EXPRESSION),\
	unsigned int: (Unsigned)(EXPRESSION),\
	signed long int: (Integer)(EXPRESSION),\
	unsigned long int: (Unsigned)(EXPRESSION),\
	signed long long int: (Integer)(EXPRESSION),\
	unsigned long long int: (Unsigned)(EXPRESSION),\
	float: (Double)(EXPRESSION),\
	double: (Double)(EXPRESSION),\
	long double: (Double)(EXPRESSION))
	//BUG? default: (Pointer)(EXPRESSION))
#else
#define Scalar(EXPRESSION) _Generic((EXPRESSION),\
	_Bool: (Scalar){.u=(EXPRESSION)},\
	char: (Scalar){.u=(EXPRESSION)},\
	signed char: (Scalar){.i=(EXPRESSION)},\
	unsigned char: (Scalar){.u=(EXPRESSION)},\
	signed short int: (Scalar){.i=(EXPRESSION)},\
	unsigned short int: (Scalar){.u=(EXPRESSION)},\
	signed int: (Scalar){.i=(EXPRESSION)},\
	unsigned int: (Scalar){.u=(EXPRESSION)},\
	signed long int: (Scalar){.i=(EXPRESSION)},\
	unsigned long int: (Scalar){.u=(EXPRESSION)},\
	signed long long int: (Scalar){.i=(EXPRESSION)},\
	unsigned long long int: (Scalar){.u=(EXPRESSION)},\
	float: (Scalar){.d=(EXPRESSION)},\
	double: (Scalar){.d=(EXPRESSION)},\
	long double: (Scalar){.d=(EXPRESSION)},\
	default: (Scalar){.p=(Pointer)(EXPRESSION)})
#endif

// Cast an union Scalar to the TYPE specified
#define cast(SCALAR,TYPE) _Generic(((TYPE){0}),\
	_Bool: (_Bool)(SCALAR).u,\
	char: (char)(SCALAR).u,\
	signed char: (signed char)(SCALAR).i,\
	unsigned char: (unsigned char)(SCALAR).u,\
	signed short int: (signed short int)(SCALAR).i,\
	unsigned short int: (unsigned short int)(SCALAR).u,\
	signed int: (signed int)(SCALAR).i,\
	unsigned int: (unsigned int)(SCALAR).u,\
	signed long int: (signed long int)(SCALAR).i,\
	unsigned long int: (unsigned long int)(SCALAR).u,\
	signed long long int: (signed long long int)(SCALAR).i,\
	unsigned long long int: (unsigned long long int)(SCALAR).u,\
	float: (float)(SCALAR).d,\
	double: (double)(SCALAR).d,\
	long double: (long double)(SCALAR).d,\
	default: (SCALAR).p)

#endif // vim:ai:sw=4:ts=4:syntax=cpp
