/*
 * Future
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef FUTURE_H
#define FUTURE_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "task.h"
#include "channel.h"
#include "scalar.h"

////////////////////////////////////////////////////////////////////////
// Type Future
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Future {
	bool    pending; // still not resolved?
	Scalar  result;  // memoized result
	Channel port;    // shared message box
} Future;

static inline Scalar ftr_get(Future* this);
static inline int    ftr_join(Future* this);
static inline int    ftr_set_(Future* this, Scalar x);
static inline int    ftr_spawn(Future* this, int(*root)(void*), void* argument);

// Accept any scalar type
#define ftr_set(FUTURE,EXPRESSION) ftr_set_((FUTURE), coerce(EXPRESSION))

// handy macro
#define spawn_future(F,R,...)\
	ftr_spawn(F,R,&(struct R){.future=F __VA_OPT__(,)__VA_ARGS__})

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int ftr_spawn(Future* this, int(*root)(void*), void* argument)
{
	enum { syncronous=0, asyncronous=1 };
	int err;

	this->pending = true;
	this->result.word = 0x1aFabada;
	if ((err=chn_init(&this->port, asyncronous)) == STATUS_SUCCESS) {
		if ((err=tsk_spawn(root, argument)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			chn_destroy(&this->port);
		}
	}
	return err;
}

// to be called once from the promise
static ALWAYS inline int ftr_set_(Future* this, Scalar x)
{
	assert(this->pending);
	int status = chn_send_(&this->port, x);
	return status;
}

// to be called once from the client
static inline int ftr_join(Future* this)
{
	assert(this->pending);
	int status = chn_receive(&this->port, &this->result);
	this->pending = false;
	chn_destroy(&this->port);
	return status;
}

// to be called any number of times from the client
static ALWAYS inline Scalar ftr_get(Future* this)
{
	if (this->pending) { ftr_join(this); }
	assert(!this->pending);
	return this->result;
}

#endif // FUTURE_H

// vim:ai:sw=4:ts=4:syntax=cpp
