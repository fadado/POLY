#ifndef POLY_CONDITION_H
#define POLY_CONDITION_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "lock.h"

/*
 * A thin façade renaming on top of C11 type `cnd_t`.
 */

////////////////////////////////////////////////////////////////////////
// Condition interface
////////////////////////////////////////////////////////////////////////

typedef cnd_t Condition;

static int  condition_await(Condition *const this, union Lock lock, bool(predicate)(void));
static int  condition_broadcast(Condition *const this);
static void condition_destroy(Condition *const this);
static int  condition_init(Condition *const this);
static int  condition_signal(Condition *const this);
static int  condition_wait(Condition *const this, union Lock lock);
static int  condition_wait_for(Condition *const this, union Lock lock, Clock duration);

////////////////////////////////////////////////////////////////////////
// Condition implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int
condition_init (Condition *const this)
{
	return cnd_init(this);
}

static ALWAYS inline void
condition_destroy (Condition *const this)
{
	cnd_destroy(this);
}

static ALWAYS inline int
condition_signal (Condition *const this)
{
	return cnd_signal(this);
}

static ALWAYS inline int
condition_wait (Condition *const this, union Lock lock)
{
	return cnd_wait(this, lock.mutex);
}

static ALWAYS inline int
condition_await (Condition *const this, union Lock lock, bool(predicate)(void))
{
	while (!predicate()) {
		int const err = cnd_wait(this, lock.mutex);
		if (err != STATUS_SUCCESS) {
			return err;
		}
	}
	return STATUS_SUCCESS;
}

static ALWAYS inline int
condition_broadcast (Condition *const this)
{
	return cnd_broadcast(this);
}

static inline int
condition_wait_for (Condition *const this, union Lock lock, Clock duration)
{
	const Clock  t  = now() + duration; // Clock ticks are nanoseconds
	const time_t s  = ns2s(t);
	const long   ns = t - s2ns(s);
	return cnd_timedwait(this, lock.mutex, &(struct timespec){.tv_sec=s, .tv_nsec=ns});
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
