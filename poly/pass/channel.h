#ifndef POLY_CHANNEL_H
#define POLY_CHANNEL_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/condition.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"
#include "../scalar.h"
#include "_fifo.h"

////////////////////////////////////////////////////////////////////////
// Channel interface
////////////////////////////////////////////////////////////////////////

typedef struct Channel {
	Lock syncronized;
	union {
		// blocking channel
		Notice board[2];
		// shared channel
		struct {
			Condition non_empty;
			Condition non_full;
		};
	};
	atomic(unsigned) flags;
	unsigned capacity;
	unsigned occupation;
	union {
		FIFO   queue; // for capacity >  1
		Scalar value; // for capacity <= 1
	};
} Channel;

static void channel_close(Channel *const this);
static void channel_destroy(Channel *const this);
static bool channel_dry(Channel const*const this);
static int  channel_init(Channel *const this, unsigned capacity);
static bool channel_ready(Channel const*const this);
static int  channel_receive(Channel *const this, Scalar *const request);
static int  channel_send(Channel *const this, Scalar scalar);

////////////////////////////////////////////////////////////////////////
// Channel implementation
////////////////////////////////////////////////////////////////////////

// Constants for flags
enum channel_flag {
	CHANNEL_BLOCKING  = (1<<0),
	CHANNEL_BUFFERED  = (1<<1),
	CHANNEL_SHARED    = (1<<2),
	CHANNEL_CLOSED    = (1<<3),
	CHANNEL_DRY       = (1<<4),
};

#ifdef DEBUG

#	define ASSERT_CHANNEL_INVARIANT\
		assert(this->occupation <= this->capacity);\
		/* flags invariants */\
		assert(!((CHANNEL_BLOCKING & this->flags) && (CHANNEL_SHARED & this->flags)));\
		assert(!(CHANNEL_BUFFERED & this->flags) || (CHANNEL_SHARED & this->flags));\
		assert(!(CHANNEL_DRY & this->flags) || (CHANNEL_CLOSED & this->flags));
#else
#	define ASSERT_CHANNEL_INVARIANT
#endif

static int
channel_init (Channel *const this, unsigned capacity)
{
	int err;

	this->occupation = this->flags = 0;
	this->capacity = capacity;
	err = lock_init(&this->syncronized);
	if (err != STATUS_SUCCESS) { return err; }

	switch (this->capacity) {
		case 0:
			this->flags |= CHANNEL_BLOCKING;
			this->capacity = 1;
			catch (board_init(2, this->board, &this->syncronized));
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
	lock_release(&this->syncronized);
	return err;
}

static void
channel_destroy (Channel *const this)
{
	assert(this->occupation == 0); // empty

	if (CHANNEL_BLOCKING & this->flags) {
		board_destroy(2, this->board);
	} else {
		assert(CHANNEL_SHARED & this->flags);
		condition_destroy(&this->non_full);
		condition_destroy(&this->non_empty);
		if (CHANNEL_BUFFERED & this->flags) {
			fifo_destroy(&this->queue);
		}
	}
	lock_destroy(&this->syncronized);
}

////////////////////////////////////////////////////////////////////////

static inline void
channel_close (Channel *const this)
{
	this->flags |= CHANNEL_CLOSED;
	if (this->occupation == 0) {
		this->flags |= CHANNEL_DRY;
	}
}

static ALWAYS inline bool
channel_dry (Channel const*const this)
{
	return (CHANNEL_DRY & this->flags);
}

static ALWAYS inline bool
channel_ready (Channel const*const this)
{
	return this->occupation != 0; // thread safe?
}

////////////////////////////////////////////////////////////////////////

static inline int
channel_send (Channel *const this, Scalar scalar)
{
	if (CHANNEL_CLOSED & this->flags) {
		panic("cannot send to a closed channel");
	}

	int err;
	enter_monitor(this);

	if (CHANNEL_BLOCKING & this->flags) {
		auto void thunk(void) {
			this->value = scalar;
		}
		catch (board_send(this->board, thunk));
		++this->occupation;
	} else if (CHANNEL_SHARED & this->flags) {
		auto bool non_full(void) {
			return this->occupation < this->capacity;
		}
		catch (condition_await(&this->non_full, &this->syncronized, non_full));

		if (CHANNEL_BUFFERED & this->flags) {
			fifo_put(&this->queue, scalar);
		} else {
			this->value = scalar;
		}
		++this->occupation;

		catch (condition_notify(&this->non_empty));
	} else {
		assert(internal_error);
	}
	ASSERT_CHANNEL_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static inline int
channel_receive (Channel *const this, Scalar *const request)
{
	assert(request != NULL);

	if (CHANNEL_DRY & this->flags) {
		*request = Unsigned(0x0);
		return STATUS_SUCCESS;
	}

	int err;
	enter_monitor(this);

	if (CHANNEL_BLOCKING & this->flags) {
		catch (board_receive(this->board));
		*request = this->value;
		--this->occupation;
	} else if (CHANNEL_SHARED & this->flags) {
		auto bool non_empty(void) { 
			return this->occupation > 0;
		}
		catch (condition_await(&this->non_empty, &this->syncronized, non_empty));

		if (CHANNEL_BUFFERED & this->flags) {
			*request = fifo_get(&this->queue);
		} else {
			*request = this->value;
		}
		--this->occupation;

		catch (condition_notify(&this->non_full));
	} else {
		assert(internal_error);
	}
	if (this->occupation == 0) {
		if (CHANNEL_CLOSED & this->flags) {
			this->flags |= CHANNEL_DRY;
		}
	}
	ASSERT_CHANNEL_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#undef ASSERT_CHANNEL_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
