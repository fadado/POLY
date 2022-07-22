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
	unsigned	flag;
} Event;

static void event_destroy(Event *const this);
static int  event_init(Event *const this);
static int  event_wait(Event *const this);
static int  event_signal(Event *const this);
static int  event_reset(Event *const this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

enum { EVENT_DOWN, EVENT_UP };

#ifdef DEBUG
#   define ASSERT_EVENT_INVARIANT \
        assert(this->flag < 2);
#else
#   define ASSERT_EVENT_INVARIANT
#endif

static int
event_init (Event *const this)
{
	this->flag = EVENT_DOWN;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=condition_init(&this->queue)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_EVENT_INVARIANT

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
 * catch (event_wait(&event)); | catch (event_signal(&event));
 */

static int
event_wait (Event *const this)
{
	MONITOR_ENTRY

	while (this->flag == EVENT_DOWN) {
		catch (condition_wait(&this->queue, &this->syncronized));
	}
	ASSERT_EVENT_INVARIANT

	ENTRY_END
}

static int
event_signal (Event *const this)
{
	MONITOR_ENTRY

	this->flag = EVENT_UP;
	catch (condition_broadcast(&this->queue));
	ASSERT_EVENT_INVARIANT

	ENTRY_END
}

static inline int
event_reset (Event *const this)
{
	MONITOR_ENTRY

	this->flag = EVENT_DOWN;

	ENTRY_END
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
