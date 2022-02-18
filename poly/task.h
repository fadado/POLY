#ifndef TASK_H
#define TASK_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "scalar.h"
#include "thread.h"
#include "passing/channel.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Task {
	bool    finished;// still not finished?
	Scalar  result;  // memoized result
	Channel mbox;    // message box
} Task;

static Scalar task_get(Task *const this);
static int    task_join(Task *const this);
static int    task_set(Task *const this, Scalar x);
static int    task_spawn(Task *const this, int main(void*), void* argument);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

/* atomic(bool) finished?
 * static inline bool finished(Task *const this) { return this->finished; }
 */

static inline int
task_spawn (Task *const this, int main(void*), void* argument)
{
	enum { syncronous=0, asyncronous=1 };
	int err;

	this->finished = false;
	this->result = (Scalar)(Unsigned)0xFabada;
	if ((err=channel_init(&this->mbox, syncronous)) == STATUS_SUCCESS) {
		if ((err=thread_spawn(main, argument)) == STATUS_SUCCESS) {
			/*skip*/;
		} else {
			channel_destroy(&this->mbox);
		}
	}
	return err;
}

// to be called once from the client
static inline int
task_join (Task *const this)
{
	assert(!this->finished);
	int status = channel_receive(&this->mbox, &this->result);
	this->finished = true;
	channel_destroy(&this->mbox);
	return status;
}

// to be called once from the promise
static ALWAYS inline int
task_set (Task *const this, Scalar x)
{
	assert(!this->finished);
	return channel_send(&this->mbox, x);
}

// to be called any number of times from the client
static ALWAYS inline Scalar
task_get (Task *const this)
{
	if (!this->finished) { task_join(this); }
	return this->result;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
