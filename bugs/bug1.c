// Bug #1
// gcc -Wall -O2 bug1.c

#include <assert.h>

#define COERCE(X)\
_Generic((X),\
	int: (signed long)(X),\
	float: (double)(X),\
	double: (double)(X),\
	long double: (double)(X),\
  	default: (void*)(unsigned long)(X)\
)

int main(int argc, char** argv)
{
	int i = 777;

	signed j = COERCE(i);
	assert(j);

	double f = 777.777;
	double d = COERCE(f);
	assert(d == 777.777);

/* clang:
tests/bug1.c:43:11: error: pointer cannot be cast to type 'double'
        int* p = COERCE(&i);
                 ^~~~~~~~~~
tests/bug1.c:9:17: note: expanded from macro 'COERCE'
        float: (double)(X),\
                       ^~~
 */

/* gcc:
tests/bug1.c: In function ‘main’:
tests/bug1.c:43:9: error: pointer value used where a floating-point was expected
   43 |         int* p = COERCE(&i);
      |         ^~~
tests/bug1.c:43:9: error: pointer value used where a floating-point was expected
tests/bug1.c:43:9: error: pointer value used where a floating-point was expected
 */
	int* p = COERCE(&i);
	assert(*p == 777);

	return 0;
}

// vim:ai:sw=4:ts=4:syntax=cpp
