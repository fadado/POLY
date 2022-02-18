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

#define TRANSPARENT __attribute__((__transparent_union__))

typedef union TRANSPARENT Scalar {
	Signed   i;
	Unsigned u;
	Double   d;
	Pointer  p;
} Scalar;

#define scalar(EXPRESSION) _Generic((EXPRESSION),\
	_Bool: (union Scalar)(Unsigned)(EXPRESSION),\
	char: (union Scalar)(Unsigned)(EXPRESSION),\
	signed char: (union Scalar)(Signed)(EXPRESSION),\
	unsigned char: (union Scalar)(Unsigned)(EXPRESSION),\
	signed short int: (union Scalar)(Signed)(EXPRESSION),\
	unsigned short int: (union Scalar)(Unsigned)(EXPRESSION),\
	signed int: (union Scalar)(Signed)(EXPRESSION),\
	unsigned int: (union Scalar)(Unsigned)(EXPRESSION),\
	signed long int: (union Scalar)(Signed)(EXPRESSION),\
	unsigned long int: (union Scalar)(Unsigned)(EXPRESSION),\
	signed long long int: (union Scalar)(Signed)(EXPRESSION),\
	unsigned long long int: (union Scalar)(Unsigned)(EXPRESSION),\
	float: (union Scalar)(Double)(EXPRESSION),\
	double: (union Scalar)(Double)(EXPRESSION),\
	long double: (union Scalar)(Double)(EXPRESSION),\
	default: (union Scalar)(Pointer)(Unsigned)(EXPRESSION)\
)

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

int main()
{
	Scalar s1;

	s1 = scalar(7.7L);
	assert(s1.d == 7.7);
	s1 = scalar(7.7F);
	assert(s1.d == 7.7F);
	s1 = scalar(7.7F);
	assert(s1.d == 7.7F);

	s1 = scalar((void*)&s1);
	assert(s1.p == &s1);


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
