#ifndef POLY_H
#define POLY_H

#include <stdbool.h>
#include <threads.h> // include <time.h>

////////////////////////////////////////////////////////////////////////
// Error management
////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <error.h>

#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>

// for assert unconditional failure
enum {
	not_implemented = 0,
	internal_error  = 0,
};

// message to stderr
#define warn(...) error(0, 0, __VA_ARGS__)

// die with a message
#define panic(FMT,...)\
	error_at_line(~0,EPERM,__FILE__,__LINE__,FMT __VA_OPT__(,)__VA_ARGS__)

// aliases
enum {
	STATUS_SUCCESS  = thrd_success,
	STATUS_BUSY     = thrd_busy,
	STATUS_ERROR    = thrd_error,
	STATUS_NOMEM    = thrd_nomem,
	STATUS_TIMEDOUT = thrd_timedout,
};

////////////////////////////////////////////////////////////////////////
// Other facilities
////////////////////////////////////////////////////////////////////////

// GCC optimization
#ifdef __GNUC__
#	define ALWAYS __attribute__((always_inline))
#	define TRANSPARENT __attribute__((__transparent_union__))
#	define fallthrough __attribute__((fallthrough))
#else
#	define ALWAYS       /*NOP*/
#	define TRANSPARENT  /*NOP*/
#	define fallthrough  /*NOP*/
#endif

#define atomic(T)  _Atomic T

////////////////////////////////////////////////////////////////////////
// Time measured in nanoseconds
////////////////////////////////////////////////////////////////////////

typedef unsigned long long Time;

#define s2ns(T)     (Time)((T)*1000000000ull)
#define ms2ns(T)    (Time)((T)*1000000ull)
#define us2ns(T)    (Time)((T)*1000ull)
#define ns2s(T)     (Time)((T)/1000000000ull)
#define ns2ms(T)    (Time)((T)/1000000ull)
#define ns2us(T)    (Time)((T)/1000ull)

static ALWAYS inline Time
now (void)
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return s2ns(ts.tv_sec) + ts.tv_nsec;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
