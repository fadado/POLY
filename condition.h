/*
 * Condition
 *
 */
#ifndef CONDITION_H
#define CONDITION_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"

////////////////////////////////////////////////////////////////////////
// Type Event
// Interface
////////////////////////////////////////////////////////////////////////

typedef cnd_t Condition;

static ALWAYS inline int  condition_broadcast(Condition* this);
static ALWAYS inline void condition_destroy(Condition* this);
static ALWAYS inline int  condition_init(Condition* this);
static ALWAYS inline int  condition_notify(Condition* this);
static ALWAYS inline int  condition_wait(Condition* this, union Lock lock);
static ALWAYS inline int  condition_wait_for(Condition* this, union Lock lock, unsigned long long nanoseconds);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static ALWAYS inline int condition_init(Condition* this)
{ return cnd_init(this); }

static ALWAYS inline void condition_destroy(Condition* this)
{ cnd_destroy(this); }

static ALWAYS inline int condition_notify(Condition* this)
{ return cnd_signal(this); }

static ALWAYS inline int  condition_wait(Condition* this, union Lock lock)
{ return cnd_wait(this, lock.mutex); }

static ALWAYS inline int condition_broadcast(Condition* this)
{ return cnd_broadcast(this); }

static ALWAYS inline int condition_wait_for(Condition* this, union Lock lock, unsigned long long nanoseconds)
{
	nanoseconds += now(); // TIME_UTC based absolute calendar time point
	time_t s  = ns2s(nanoseconds);
	long   ns = nanoseconds - s2ns(s);
	return cnd_timedwait(this, lock.mutex,
						 &(struct timespec){.tv_sec=s, .tv_nsec=ns});
}

#endif // LOCK_H

// vim:ai:sw=4:ts=4:syntax=cpp
