#ifndef __GNUC__
#error I need the GNU C compiler. Sorry.
#endif
#ifndef POLY_H
#define POLY_H

/* Module parameters:
 *     DEBUG
 */

#ifndef DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h> // include <time.h>

////////////////////////////////////////////////////////////////////////
// Error management
////////////////////////////////////////////////////////////////////////

// output to stderr
#define warn(...) error(0, 0, __VA_ARGS__)

// die with an output text
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

// error management strategy (assume `int err;` defined)
#define catch(X)\
	if ((err=(X)) != STATUS_SUCCESS) {\
		goto onerror;\
	}

////////////////////////////////////////////////////////////////////////
// Other facilities
////////////////////////////////////////////////////////////////////////

// force inlining for functions
#define ALWAYS      __attribute__((always_inline))

// handy trick: see `scalar.h` and `lock.h` for examples
#define TRANSPARENT __attribute__((__transparent_union__))

// disable warnings on `case:...no break...fallthrough;case:` 
#define fallthrough __attribute__((fallthrough))

// sequential consistency
#define atomic(T)   _Atomic(T)

////////////////////////////////////////////////////////////////////////
// Clock time measured in nanoseconds
////////////////////////////////////////////////////////////////////////

typedef unsigned long long Clock;

#define s2ns(T)     (Clock)((T)*1000000000ull)
#define ms2ns(T)    (Clock)((T)*1000000ull)
#define us2ns(T)    (Clock)((T)*1000ull)
#define ns2s(T)     (Clock)((T)/1000000000ull)
#define ns2ms(T)    (Clock)((T)/1000000ull)
#define ns2us(T)    (Clock)((T)/1000ull)

// TIME_UTC based absolute calendar time point
static inline Clock
now (void)
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return s2ns(ts.tv_sec) + ts.tv_nsec;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
