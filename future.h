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
	short   status;  // -1: pending; 0: OK; >0: error
	Scalar  result;  // memoized result
	Channel channel;
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

static ALWAYS inline bool _ftr_pending(Future* self)
{ return self->status == -1; }

static ALWAYS inline bool _ftr_resolved(Future* self)
{ return self->status >= 0; }

static inline int ftr_run(Future* self, int(*root)(void*), void* argument)
{
	int err;

	self->status = -1;
	if ((err=chn_init(&self->channel, 0)) == STATUS_SUCCESS) {
		if ((err=tsk_run(root, (void*[2]){self, argument})) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			chn_destroy(&self->channel);
		}
	}
	return err;
}

static inline int ftr_wait(Future* self)
{
	if (_ftr_pending(self)) {
		self->status = chn_receive(&self->channel, &self->result);
		chn_destroy(&self->channel);
	}
	assert(_ftr_resolved(self));
	return self->status;
}

static ALWAYS inline int ftr_set_(Future* self, Scalar x)
{
	assert(_ftr_pending(self));
	return chn_send_(&self->channel, x); // ??? chn_send_ or chn_send ???
}

static ALWAYS inline Scalar ftr_get(Future* self)
{
	if (_ftr_pending(self)) { ftr_wait(self); }
	return self->result;
}

#endif // FUTURE_H

// vim:ai:sw=4:ts=4:syntax=cpp
