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
	bool    pending;
	Scalar  result;  // memoized result
	Channel port;
} Future;

static inline int    ftr_run(Future* self, int(*root)(void*), void* argument);
static inline int    ftr_wait(Future* self);
static inline int    ftr_set_(Future* self, Scalar x);
static inline Scalar ftr_get(Future* self);

// Accept any scalar type
#define ftr_set(FUTURE,EXPRESSION) ftr_set_((FUTURE), coerce(EXPRESSION))

// handy macro
#define ftr_spawn(F,R,...)  ftr_run(F, R, &(struct R){__VA_ARGS__})

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int ftr_run(Future* self, int(*root)(void*), void* argument)
{
	enum { syncronous=0, asyncronous=1 };
	int err;

	self->pending = true;
	self->result.word = 0x1aFabada;
	if ((err=chn_init(&self->port, asyncronous)) == STATUS_SUCCESS) {
		if ((err=tsk_run(root, (void*[2]){argument, self})) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			chn_destroy(&self->port);
		}
	}
	return err;
}

// to be called once from the promise
static ALWAYS inline int ftr_set_(Future* self, Scalar x)
{
	assert(self->pending);
	int status = chn_send_(&self->port, x);               /*ASYNC*/
	return status;
}

// to be called once from the client
static inline int ftr_wait(Future* self)
{
	assert(self->pending);
	int status = chn_receive(&self->port, &self->result); /*ASYNC*/
	self->pending = false;
	chn_destroy(&self->port);
	return status;
}

// to be called any number of times from the client
static ALWAYS inline Scalar ftr_get(Future* self)
{
	if (self->pending) { ftr_wait(self); }
	assert(!self->pending);
	return self->result;
}

////////////////////////////////////////////////////////////////////////
// Experimental structures
////////////////////////////////////////////////////////////////////////

#define PROMISE_BEGIN(TASK_NAME)\
	};\
	static int TASK_NAME(void* a_) {\
		struct TASK_NAME* self = ((void**)a_)[0];\
		struct Future* future  = ((void**)a_)[1];

#endif // FUTURE_H

// vim:ai:sw=4:ts=4:syntax=cpp
