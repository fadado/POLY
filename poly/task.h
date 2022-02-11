#ifndef TASK_H
#define TASK_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "scalar.h"
#include "thread.h"
#include "channel.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Future {
	bool    pending; // still not finished?
	Scalar  result;  // memoized result
	Channel port;    // shared message box
} Future;

static inline int    task_join(Future* this);
static inline int    task_spawn(Future* this, int(*root)(void*), void* argument);

static inline int    future_set(Future* this, Scalar x);
static inline Scalar future_get(Future* this);

// handy macro
#define spawn_task(F,R,...)\
	task_spawn(F,R,&(struct R){.future=F __VA_OPT__(,)__VA_ARGS__})

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/* atomic(bool) pending?
 * static inline bool finished(Future* this) { return !this->pending; }
 */

static inline int task_spawn(Future* this, int(*root)(void*), void* argument)
{
	enum { syncronous=0, asyncronous=1 };
	int err;

	this->pending = true;
	this->result = (Scalar)(Unsigned)0xFabadaUL;
	if ((err=channel_init(&this->port, syncronous)) == STATUS_SUCCESS) {
		if ((err=thread_spawn(root, argument)) == STATUS_SUCCESS) {
			/*skip*/;
		} else {
			channel_destroy(&this->port);
		}
	}
	return err;
}

// to be called once from the client
static inline int task_join(Future* this)
{
	assert(this->pending);
	int status = channel_receive(&this->port, &this->result);
	this->pending = false;
	channel_destroy(&this->port);
	return status;
}

// to be called once from the promise
static ALWAYS inline int future_set(Future* this, Scalar x)
{
	assert(this->pending);
	return channel_send(&this->port, x);
}

// to be called any number of times from the client
static ALWAYS inline Scalar future_get(Future* this)
{
	if (this->pending) { task_join(this); }
	return this->result;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
