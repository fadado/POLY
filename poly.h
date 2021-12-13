/*
 * POLY
 *
 */
#ifndef POLY_H
#define POLY_H

#include <errno.h>
#include <error.h>
#include <err.h>

#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>

// GCC optimization
#ifdef __GNUC__
#	define ALWAYS __attribute__((always_inline))
#else
#	define ALWAYS /*NOP*/
#endif

// for assert unconditional failure
enum {
	not_implemented = 0,
	internal_error  = 0,
};

//
// Ahhhhhhhhhhhg!
//
#define panic(FMT,...)\
	error_at_line(~0,EPERM,__FILE__,__LINE__,FMT __VA_OPT__(,)__VA_ARGS__)

// To easy callbacks
typedef void (*Thunk)(void);

// Warnings while debugging
#ifdef DEBUG
#define trace(...) warn(__VA_ARGS__)
#else
#define trace(...)
#endif

#endif // POLY_H

// vim:ai:sw=4:ts=4:syntax=cpp
