#ifndef CHANNEL_H
#define CHANNEL_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../scalar.h"
#include "../array/fifo.h"
#include "../monitor/lock.h"
#include "../monitor/condition.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"

////////////////////////////////////////////////////////////////////////
// Type Channel (of scalars)
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	atomic(unsigned) flags;
	unsigned capacity;
	unsigned occupation;
	union {
		FIFO   queue; // for capacity >  1
		Scalar value; // for capacity <= 1
	};
	Lock monitor; // TODO: use RecursiveLock?
	union {
		// blocking channel
		Notice board[2];
		// shared channel
		struct {
			Condition non_empty;
			Condition non_full;
		};
	};
} Channel;

static void channel_close(Channel *const this);
static void channel_destroy(Channel *const this);
static bool channel_drained(Channel const*const this);
static int  channel_init(Channel *const this, unsigned capacity);
static int  channel_receive(Channel *const this, Scalar *const request);
static int  channel_send(Channel *const this, Scalar scalar);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

// Constants for flags
enum channel_flag {
	CHANNEL_BLOCKING  = (1<<0),
	CHANNEL_BUFFERED  = (1<<1),
	CHANNEL_SHARED    = (1<<2),
	CHANNEL_CLOSED    = (1<<3),
	CHANNEL_DRAINED   = (1<<4),
};

#ifdef DEBUG
#	define ASSERT_CHANNEL_INVARIANT\
		assert(this->occupation <= this->capacity);
#else
#	define ASSERT_CHANNEL_INVARIANT
#endif

// Error management strategy for this module
#define catch(X)\
	if ((err=(X)) != STATUS_SUCCESS)\
		goto onerror

//
// Private predicates
//
static ALWAYS inline bool
_channel_empty (Channel const*const this)
{
	return (this->occupation == 0);
}

static ALWAYS inline bool
_channel_full (Channel const*const this)
{
	return (this->occupation == this->capacity);
}

//
// Channel life
//
static int
channel_init (Channel *const this, unsigned capacity)
{
	int err;

	this->occupation = this->flags = 0;
	this->capacity = capacity;
	err = lock_init(&this->monitor);
	if (err != STATUS_SUCCESS) { return err; }

	switch (this->capacity) {
		case 0:
			this->flags |= CHANNEL_BLOCKING;
			catch (board_init(this->board, 2, &this->monitor));
			this->capacity = 1;
			break;
		default: // > 1
			this->flags |= CHANNEL_BUFFERED;
			catch (fifo_init(&this->queue, capacity));
			fallthrough; // do not break
		case 1:
			this->flags |= CHANNEL_SHARED;
			err = condition_init(&this->non_empty);
			if (err != STATUS_SUCCESS) {
				if (CHANNEL_BUFFERED & this->flags) {
					fifo_destroy(&this->queue);
				}
				goto onerror;
			}
			err = condition_init(&this->non_full);
			if (err != STATUS_SUCCESS) {
				condition_destroy(&this->non_empty);
				if (CHANNEL_BUFFERED & this->flags) {
					fifo_destroy(&this->queue);
				}
				goto onerror;
			}
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

static void
channel_destroy (Channel *const this)
{
	assert(_channel_empty(this)); // TODO: require emptyness???

	if (CHANNEL_BLOCKING & this->flags) {
		board_destroy(this->board, 2);
	} else {
		assert(CHANNEL_SHARED & this->flags);
		condition_destroy(&this->non_full);
		condition_destroy(&this->non_empty);
		if (CHANNEL_BUFFERED & this->flags) {
			fifo_destroy(&this->queue);
		}
	}
	lock_destroy(&this->monitor);
}

static inline void
channel_close (Channel *const this)
{
	this->flags |= CHANNEL_CLOSED;
	//?lock_acquire(&this->monitor);
	if (this->occupation == 0) {
		this->flags |= CHANNEL_DRAINED;
	}
	//?lock_release(&this->monitor);
}

static ALWAYS inline bool
channel_drained (Channel const*const this)
{
	return (CHANNEL_DRAINED & this->flags);
}

//
// Monitor helpers
//
#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	if ((err=lock_acquire(&this->monitor))!=STATUS_SUCCESS) {\
		return err;\
	}\
	if (CHANNEL_SHARED & this->flags) {\
		while (PREDICATE(this)) {\
			if ((err=condition_wait(&CONDITION, &this->monitor))!=STATUS_SUCCESS) {\
				goto onerror;\
			}\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if (CHANNEL_SHARED & this->flags) {\
		if ((err=condition_notify(&CONDITION))!=STATUS_SUCCESS) {\
			goto onerror;\
		}\
	}\
	if ((err=lock_release(&this->monitor))!=STATUS_SUCCESS) {\
		return err;\
	}

//
// Monitor entries
//
static inline int
channel_send (Channel *const this, Scalar scalar)
{
	if (CHANNEL_CLOSED & this->flags) {
		panic("channel_send cannot use a closed channel");
	}

	int err;
	ENTER_CHANNEL_MONITOR (_channel_full, this->non_full)

	if (CHANNEL_BLOCKING & this->flags) {
		void thunk(void) {
			this->value = scalar;
		}
		catch (board_send(this->board, thunk));
	} else if (CHANNEL_BUFFERED & this->flags) {
		fifo_put(&this->queue, scalar);
	} else {
		this->value = scalar;
	}
	++this->occupation;
	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (this->non_empty)
	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

static inline int
channel_receive (Channel *const this, Scalar *const request)
{
	inline ALWAYS void receive(Scalar s) {
		if (request != NULL) { *request = s; }
	}
	if (CHANNEL_DRAINED & this->flags) {
		receive(0X0U);
		return STATUS_SUCCESS;
	}

	int err;
	ENTER_CHANNEL_MONITOR (_channel_empty, this->non_empty)

	if (CHANNEL_BLOCKING & this->flags) {
		catch (board_receive(this->board));
		receive(this->value);
	} else if (CHANNEL_BUFFERED & this->flags) {
		receive(fifo_get(&this->queue));
	} else {
		receive(this->value);
	}
	--this->occupation;
	ASSERT_CHANNEL_INVARIANT

	if ((CHANNEL_CLOSED & this->flags) && this->occupation == 0) {
		this->flags |= CHANNEL_DRAINED;
	}

	LEAVE_CHANNEL_MONITOR (this->non_full)
	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

#undef catch
#undef ASSERT_CHANNEL_INVARIANT
#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
