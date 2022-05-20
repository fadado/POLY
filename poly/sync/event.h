#ifndef POLY_EVENT_H
#define POLY_EVENT_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/condition.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Event {
	Lock    	syncronized;
	Condition	queue;
	bool   		signaled;
} Event;

static void event_destroy(Event *const this);
static int  event_init(Event *const this);
static int  event_wait(Event *const this);
static int  event_signal(Event *const this);
static int  event_reset(Event *const this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static int
event_init (Event *const this)
{
	this->signaled = false;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=condition_init(&this->queue)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	return STATUS_SUCCESS;
}

static void
event_destroy (Event *const this)
{
	condition_destroy(&this->queue);
	lock_destroy(&this->syncronized);
}

/*
 * catch (event_init(&event));
 * ...
 * catch (event_wait(&event)) | catch (event_signal(&event))
 */

static inline int
event_wait (Event *const this)
{
	int err;
	enter_monitor(this);

	while (!this->signaled) {
		catch (condition_wait(&this->queue, &this->syncronized));
	}

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static inline int
event_signal (Event *const this)
{
	int err;
	enter_monitor(this);

	this->signaled = true;
	catch (condition_broadcast(&this->queue));

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static int
event_reset (Event *const this)
{
	int err;
	enter_monitor(this);

	this->signaled = false;

	leave_monitor(this);
	return STATUS_SUCCESS;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
