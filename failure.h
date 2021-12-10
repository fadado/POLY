/*
 * To cope with failure.
 *
 * Typical usage:
 *
 *  // uncomment next line to enable assertions
 *  //#define DEBUG
 *  #include "failure.h"
 *
 */
#ifndef FAILURE_H
#define FAILURE_H

#include <errno.h>
#include <error.h>
#include <err.h>

#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>

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
#define WARN(...) warn(__VA_ARGS__)
#else
#define WARN(...)
#endif

#endif // FAILURE_H

// vim:ai:sw=4:ts=4:syntax=cpp
