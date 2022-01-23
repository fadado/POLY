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

typedef signed long long   Integer;
typedef unsigned long long Unsigned;
typedef double             Double;
typedef void*              Pointer;

static_assert(sizeof(Double) == sizeof(Integer));
static_assert(sizeof(Double) == sizeof(Unsigned));
static_assert(sizeof(Double) == sizeof(Pointer));
static_assert(sizeof(Double) == 8);

typedef union TRANSPARENT Scalar {
	Integer  integer;	// _i?
	Unsigned word;		// _u?
	Double   real;		// _d?
	Pointer  pointer;	// _p?
} Scalar;

static_assert(sizeof(Scalar) == 8);

////////////////////////////////////////////////////////////////////////
// Cast to and from Scalar
////////////////////////////////////////////////////////////////////////

#ifdef TheCoerceMacroOrTheCompilerHaveBugs

// Cast from any native scalar EXPRESSION to an Scalar
#define coerce(EXPRESSION) (Scalar)_Generic((EXPRESSION),\
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

//	this causes an error!!!
//	default: (Pointer)(Unsigned)(EXPRESSION)) ???

#endif

// alternative to coerce
#define Integer(x)    (Scalar)(Integer)(x)
#define Unsigned(x)   (Scalar)(Unsigned)(x)
#define Double(x)     (Scalar)(Double)(x)
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
