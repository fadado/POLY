#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h> // calloc

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../scalar.h"
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
	unsigned front;
	union {
		Scalar* buffer; // for capacity > 1
		Scalar  value;  // for capacity == 1
	};
	Lock monitor; // RecursiveLock?
	union {
		// buffered channel
		struct {
			Condition non_empty;
			Condition non_full;
		};
		// blocking channel
		Notice board[2];
	};
} Channel;

static void channel_close(Channel *const this);
static void channel_destroy(Channel *const this);
static bool channel_drained(Channel const*const this);
static int  channel_init(Channel *const this, unsigned capacity);
static int  channel_receive(Channel *const this, Scalar* message);
static int  channel_send(Channel *const this, Scalar message);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

// Constants for flags
enum channel_flag {
	CHANNEL_BUFFERED  = (1<<0),
	CHANNEL_BLOCKING  = (1<<1),
	CHANNEL_CLOSED    = (1<<2),
	CHANNEL_DRAINED   = (1<<3),
};

#ifdef DEBUG
#	define ASSERT_CHANNEL_INVARIANT\
		assert(0 <= this->occupation && this->occupation <= this->capacity);\
		if (this->capacity > 1) {\
			assert(0 <= this->front && this->front <  this->capacity);\
			assert(this->buffer != (Scalar*)0);\
		}\
		assert(!(_channel_empty(this) && _channel_full(this)));
#else
#	define ASSERT_CHANNEL_INVARIANT
#endif

// Same error management strategy in all this module
#define catch(X)\
	if ((err=(X)) != STATUS_SUCCESS)\
		goto onerror

//
// Predicates
//
static ALWAYS inline bool
_channel_empty (Channel const*const this)
{
	return this->occupation == 0;
}

static ALWAYS inline bool
_channel_full (Channel const*const this)
{
	return this->occupation == this->capacity;
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
			err = board_init(this->board, 2, &this->monitor);
			if (err != STATUS_SUCCESS) {
				lock_destroy(&this->monitor);
				return err;
			}
			this->capacity = 1;
			this->flags |= CHANNEL_BLOCKING;
			break;
		default: // > 1
			this->front = 0;
			this->buffer = calloc(this->capacity, sizeof(Scalar));
			if (!this->buffer) {
				lock_destroy(&this->monitor);
				return err;
			}
			fallthrough;
		case 1:
			err = condition_init(&this->non_empty);
			if (err != STATUS_SUCCESS) {
				if (this->capacity > 1) { free(this->buffer); }
				lock_destroy(&this->monitor);
				return err;
			}
			err = condition_init(&this->non_full);
			if (err != STATUS_SUCCESS) {
				condition_destroy(&this->non_empty);
				if (this->capacity > 1) { free(this->buffer); }
				lock_destroy(&this->monitor);
				return err;
			}
			this->flags |= CHANNEL_BUFFERED;
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	return STATUS_SUCCESS;
}

static inline void
channel_destroy (Channel *const this)
{
	assert(_channel_empty(this));

	if (this->flags & CHANNEL_BUFFERED) {
		condition_destroy(&this->non_full);
		condition_destroy(&this->non_empty);
	} else if (this->flags & CHANNEL_BLOCKING) {
		board_destroy(this->board, 2);
	}
	lock_destroy(&this->monitor);
	if (this->capacity > 1) {
		free(this->buffer);
		this->buffer = (Scalar*)0;
	}
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
	return (this->flags & CHANNEL_DRAINED);
}

//
// Monitor helpers
//
#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	if ((err=lock_acquire(&this->monitor))!=STATUS_SUCCESS) {\
		return err;\
	}\
	if (this->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else while (PREDICATE(this)) {\
		if ((err=condition_wait(&CONDITION, &this->monitor))!=STATUS_SUCCESS) {\
			lock_destroy(&this->monitor);\
			return err;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if (this->flags & CHANNEL_BLOCKING) /*NOP*/;\
	else if ((err=condition_notify(&CONDITION))!=STATUS_SUCCESS) {\
		lock_release(&this->monitor);\
		return err;\
	}\
	if ((err=lock_release(&this->monitor))!=STATUS_SUCCESS) {\
		return err;\
	}

static inline int
channel_send (Channel *const this, Scalar message)
{
	if (this->flags & CHANNEL_CLOSED) {
		panic("channel_send cannot use a closed channel");
	}

	int err;
	ENTER_CHANNEL_MONITOR (_channel_full, this->non_full)

	if (this->flags & CHANNEL_BLOCKING) {
		assert(this->capacity == 1);
		void thunk(void) { this->value = message; }
		catch (board_send(this->board, thunk));
	} else if (this->capacity == 1) {
		assert(_channel_empty(this));
		this->value = message;
	} else {
		assert(this->capacity > 1);
		this->buffer[this->front] = message;
		this->front = (this->front+1) % this->capacity;
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
channel_receive (Channel *const this, Scalar* message)
{
	if (this->flags & CHANNEL_DRAINED) {
		if (message) *message = Unsigned(0x0);
		return STATUS_SUCCESS;
	}

	int err;
	ENTER_CHANNEL_MONITOR (_channel_empty, this->non_empty)

	if (this->flags & CHANNEL_BLOCKING) {
		assert(this->capacity == 1);
		catch (board_receive(this->board));
		if (message) *message = this->value;
	} else if (this->capacity == 1) {
		assert(_channel_full(this));
		if (message) *message = this->value;
	} else if (message) {
		assert(this->capacity > 1);
		register int back = this->front - this->occupation;
		back = (back >= 0) ? back : back+this->capacity;
		*message = this->buffer[back];
	} // else ignore
	--this->occupation;
	ASSERT_CHANNEL_INVARIANT

	if ((this->flags & CHANNEL_CLOSED) && this->occupation == 0) {
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
