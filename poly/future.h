#ifndef FUTURE_H
#define FUTURE_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "scalar.h"
#include "thread.h"
#include "passing/channel.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Future {
	bool    finished;// still not finished?
	Scalar  result;  // memoized result
	Channel mbox;    // message box
} Future;

static int    future_fork(int main(void*), void* argument, Future *const this);
static Scalar future_get(Future *const this);
static int    future_join(Future *const this);
static int    future_set(Future *const this, Scalar x);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/* atomic(bool) finished ?
 * static inline bool future_finished(Future *const this) { return this->finished; }
 */

static inline int
future_fork (int main(void*), void* argument, Future *const this)
{
	enum { syncronous=0, asyncronous=1 };
	int err;

	this->finished = false;
	this->result = Unsigned(0xFabada);
	if ((err=channel_init(&this->mbox, asyncronous)) == STATUS_SUCCESS) {
		if ((err=thread_fork(main, argument, &(Thread){0})) == STATUS_SUCCESS) {
			/*skip*/;
		} else {
			channel_destroy(&this->mbox);
		}
	}
	return err;
}

// to be called once from the client
static inline int
future_join (Future *const this)
{
	assert(!this->finished);
	const int status = channel_receive(&this->mbox, &this->result);
	this->finished = true;
	channel_destroy(&this->mbox);
	return status;
}

// to be called once from the promise
static ALWAYS inline int
future_set (Future *const this, Scalar x)
{
	assert(!this->finished);
	return channel_send(&this->mbox, x);
}

// to be called any number of times from the client
static ALWAYS inline Scalar
future_get (Future *const this)
{
	if (!this->finished) { future_join(this); }
	return this->result;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
