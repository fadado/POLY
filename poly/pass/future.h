#ifndef FUTURE_H
#define FUTURE_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "../thread.h"
#include "../scalar.h"
#include "channel.h"

////////////////////////////////////////////////////////////////////////
// Future interface
////////////////////////////////////////////////////////////////////////

typedef struct Future {
	Channel inbox;    // asyncronous communication
	bool    finished; // still not finished?
	Scalar  result;   // memoized result
} Future;

static int    future_fork(int main(void*), void* argument, Future *const this);
static int    future_join(Future *const this);
static Scalar future_receive(Future *const this);
static int    future_send(Future *const this, Scalar x);

/*
 *  TASK_TYPE (name)
 *      Future* future;
 *      slots
 *      ...
 *  END_TYPE
 *
 *  TASK_BODY (name)
 *      ...
 *  END_BODY
 */
#define RUN_promise(T,F,...)\
    future_fork(T,\
        &(struct T){.future=(F)__VA_OPT__(,)__VA_ARGS__}, (F))

////////////////////////////////////////////////////////////////////////
// Future implementation
////////////////////////////////////////////////////////////////////////

/* atomic(bool) finished ?
 * static inline bool future_finished(Future *const this) { return this->finished; }
 */

static inline int
future_fork (int main(void*), void* argument, Future *const this)
{
	enum { syncronous, asyncronous };
	int err;

	this->finished = false;
	this->result   = Unsigned(0xFabada);

	if ((err=channel_init(&this->inbox, asyncronous)) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=thread_fork(main, argument, &(Thread){0})) != STATUS_SUCCESS) {
		channel_destroy(&this->inbox);
		return err;
	}
	return STATUS_SUCCESS;
}

// to be called once from the client
static inline int
future_join (Future *const this)
{
	assert(!this->finished);
	const int status = channel_receive(&this->inbox, &this->result);
	this->finished = true;
	channel_destroy(&this->inbox);
	return status;
}

// to be called once from the promise
static ALWAYS inline int
future_send (Future *const this, Scalar x)
{
	assert(!this->finished);
	return channel_send(&this->inbox, x);
}

// to be called any number of times from the client
static ALWAYS inline Scalar
future_receive (Future *const this)
{
	if (!this->finished) { future_join(this); }
	return this->result;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
