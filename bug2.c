// Bug #2
// gcc -Wall -O2 bug2.c

#include <assert.h>
#include <string.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

#define streql(s,t) (strcmp(s,t)==0)

#define TypeName(X) _Generic(X,\
	_Bool: "_Bool",\
	char: "char",\
	signed char: "signed char",\
	unsigned char: "unsigned char",\
	signed short int: "signed short int",\
	unsigned short int: "unsigned short int",\
	signed int: "signed int",\
	unsigned int: "unsigned int",\
	signed long int: "signed long int",\
	unsigned long int: "unsigned long int",\
	signed long long int: "signed long long int",\
	unsigned long long int: "unsigned long long int",\
	float: "float",\
	double: "double",\
	long double: "long double",\
	default: "UNKNOWN")

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

typedef signed long long   Signed;
typedef unsigned long long Unsigned;
typedef double             Double;
typedef void*              Pointer;

static_assert(sizeof(Double) == sizeof(Signed));
static_assert(sizeof(Double) == sizeof(Unsigned));
static_assert(sizeof(Double) == sizeof(Pointer));
static_assert(sizeof(Double) == 8);

#define ALWAYS __attribute__((always_inline))
#define TRANSPARENT __attribute__((__transparent_union__))

typedef union TRANSPARENT Scalar {
	Signed   i;
	Unsigned u;
	Double   d;
	Pointer  p;
} Scalar;

static_assert(sizeof(union Scalar) == 8);

static inline ALWAYS union Scalar scalar(union Scalar s) { return s;}

#define Scalar(EXPRESSION) (union Scalar)_Generic((EXPRESSION),\
	_Bool: (Unsigned)(EXPRESSION),\
	char: (Unsigned)(EXPRESSION),\
	signed char: (Signed)(EXPRESSION),\
	unsigned char: (Unsigned)(EXPRESSION),\
	signed short int: (Signed)(EXPRESSION),\
	unsigned short int: (Unsigned)(EXPRESSION),\
	signed int: (Signed)(EXPRESSION),\
	unsigned int: (Unsigned)(EXPRESSION),\
	signed long int: (Signed)(EXPRESSION),\
	unsigned long int: (Unsigned)(EXPRESSION),\
	signed long long int: (Signed)(EXPRESSION),\
	unsigned long long int: (Unsigned)(EXPRESSION),\
	float: (Double)(EXPRESSION),\
	double: (Double)(EXPRESSION),\
	long double: (Double)(EXPRESSION)\
)
	//default: (Pointer)(EXPRESSION))

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

int main()
{
	Scalar s1 = Scalar(1.1F);
	assert(s1.d == 1.1);



	assert(streql(TypeName((_Bool)0), "_Bool"));
	assert(streql(TypeName((char)0), "char"));
	assert(streql(TypeName((signed char)0), "signed char"));
	assert(streql(TypeName((unsigned char)0), "unsigned char"));
	assert(streql(TypeName((signed short int)0), "signed short int"));
	assert(streql(TypeName((unsigned short int)0), "unsigned short int"));
	assert(streql(TypeName((signed int)0), "signed int"));
	assert(streql(TypeName(0), "signed int"));
	assert(streql(TypeName('0'), "signed int"));
	assert(streql(TypeName((unsigned int)0), "unsigned int"));
	assert(streql(TypeName(0U), "unsigned int"));
	assert(streql(TypeName((signed long int)0), "signed long int"));
	assert(streql(TypeName((unsigned long int)0), "unsigned long int"));
	assert(streql(TypeName((signed long long int)0), "signed long long int"));
	assert(streql(TypeName((unsigned long long int)0), "unsigned long long int"));
	assert(streql(TypeName((float)0), "float"));
	assert(streql(TypeName((double)0), "double"));
	assert(streql(TypeName(0.0), "double"));
	assert(streql(TypeName(0e0), "double"));
	assert(streql(TypeName((long double)0), "long double"));
	assert(streql(TypeName((void)0), "UNKNOWN"));

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
