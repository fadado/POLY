#ifndef POLY_SCALAR_H
#define POLY_SCALAR_H

#ifndef POLY_H
#include "POLY.h"
#endif

////////////////////////////////////////////////////////////////////////
// Scalar type and 64 bit scalar types
////////////////////////////////////////////////////////////////////////

typedef signed long long   Signed;
typedef unsigned long long Unsigned;
typedef double             Double;
typedef void*              Pointer;

static_assert(sizeof(Double) == sizeof(Signed));
static_assert(sizeof(Double) == sizeof(Unsigned));
static_assert(sizeof(Double) == sizeof(Pointer));
static_assert(sizeof(Double) == 8);

typedef union POLY_TRANSPARENT Scalar {
	Signed   i;
	Unsigned u;
	Double   d;
	Pointer  p;
} Scalar;

static_assert(sizeof(union Scalar) == 8);

////////////////////////////////////////////////////////////////////////
// Cast to and from Scalar
////////////////////////////////////////////////////////////////////////

#define Signed(x)   ((union Scalar)(Signed)x)
#define Unsigned(x) ((union Scalar)(Unsigned)x)
#define Double(x)   ((union Scalar)(Double)x)
#define Pointer(x)  ((union Scalar)(Pointer)x)

// Cast an union Scalar to the TYPE specified
#define cast(SCALAR,TYPE) _Generic(((TYPE){0}),                 \
    _Bool: (_Bool)(SCALAR).u,                                   \
    char: (char)(SCALAR).u,                                     \
    signed char: (signed char)(SCALAR).i,                       \
    unsigned char: (unsigned char)(SCALAR).u,                   \
    signed short int: (signed short int)(SCALAR).i,             \
    unsigned short int: (unsigned short int)(SCALAR).u,         \
    signed int: (signed int)(SCALAR).i,                         \
    unsigned int: (unsigned int)(SCALAR).u,                     \
    signed long int: (signed long int)(SCALAR).i,               \
    unsigned long int: (unsigned long int)(SCALAR).u,           \
    signed long long int: (signed long long int)(SCALAR).i,     \
    unsigned long long int: (unsigned long long int)(SCALAR).u, \
    float: (float)(SCALAR).d,                                   \
    double: (double)(SCALAR).d,                                 \
    long double: (long double)(SCALAR).d,                       \
    default: (SCALAR).p)

#endif // vim:ai:sw=4:ts=4:syntax=cpp
