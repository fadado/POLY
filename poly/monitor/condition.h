#ifndef CONDITION_H
#define CONDITION_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef cnd_t Condition;

static ALWAYS inline int  condition_broadcast(Condition* this);
static ALWAYS inline void condition_destroy(Condition* this);
static ALWAYS inline int  condition_init(Condition* this);
static ALWAYS inline int  condition_notify(Condition* this);
static ALWAYS inline int  condition_wait(Condition* this, union Lock lock);
static ALWAYS inline int  condition_wait_for(Condition* this, union Lock lock, Time duration);

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

static inline int condition_wait_for(Condition* this, union Lock lock, Time duration)
{
	Time t = now();
	t += duration; // TIME_UTC based absolute calendar time point
	time_t s  = ns2s(t);
	long   ns = t - s2ns(s);
	return cnd_timedwait(this, lock.mutex, &(struct timespec){.tv_sec=s, .tv_nsec=ns});
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
