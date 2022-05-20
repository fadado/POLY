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
	atomic(unsigned) flags;
	unsigned mode;
	unsigned capacity;
	unsigned occupation;
	union {
		// blocking (syncronous) channel
		Notice board[2];
		// asyncronous channel
		struct {
			Condition non_empty;
			Condition non_full;
			bool      buffered;
		};
	};
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
static int  channel_receive(Channel *const this, Scalar response[static 1]);
static int  channel_send(Channel *const this, Scalar scalar);

////////////////////////////////////////////////////////////////////////
// Channel implementation
////////////////////////////////////////////////////////////////////////

// Constants for flags
enum { CHANNEL_CLOSED=0x01, CHANNEL_DRY=0x02 };

// Constants for mode
enum { CHANNEL_MODE_SYNC='S', CHANNEL_MODE_ASYNC='A' };

#ifdef DEBUG
#	define ASSERT_CHANNEL_INVARIANT\
		assert(this->occupation <= this->capacity);\
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

	switch (capacity) {
		case 0:
			this->mode = CHANNEL_MODE_SYNC;
			this->capacity = 1;
			catch (board_init(2, this->board, &this->syncronized));
			break;
		case 1:
			this->mode = CHANNEL_MODE_ASYNC;
			this->buffered = false;
			if ((err=condition_init(&this->non_empty)) != STATUS_SUCCESS) {
				goto onerror;
			}
			if ((err=condition_init(&this->non_full)) != STATUS_SUCCESS) {
				condition_destroy(&this->non_empty);
				goto onerror;
			}
			break;
		default: // > 1
			this->mode = CHANNEL_MODE_ASYNC;
			this->buffered = true;
			catch (fifo_init(&this->queue, capacity));
			if ((err = condition_init(&this->non_empty)) != STATUS_SUCCESS) {
				fifo_destroy(&this->queue);
				goto onerror;
			}
			if ((err=condition_init(&this->non_full)) != STATUS_SUCCESS) {
				condition_destroy(&this->non_empty);
				fifo_destroy(&this->queue);
				goto onerror;
			}
			break;
	}
	assert(this->mode == CHANNEL_MODE_ASYNC || this->mode == CHANNEL_MODE_SYNC);
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

	switch (this->mode) {
		case CHANNEL_MODE_SYNC:
			board_destroy(2, this->board);
			break;
		case CHANNEL_MODE_ASYNC:
			condition_destroy(&this->non_full);
			condition_destroy(&this->non_empty);
			if (this->buffered) {
				fifo_destroy(&this->queue);
			}
			break;
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

	switch (this->mode) {
		case CHANNEL_MODE_SYNC:
			auto void thunk(void) {
				this->value = scalar;
			}
			catch (board_send(this->board, thunk));
			++this->occupation;
			break;
		case CHANNEL_MODE_ASYNC:
			auto bool non_full(void) {
				return this->occupation < this->capacity;
			}
			catch (condition_await(&this->non_full, &this->syncronized, non_full));

			if (this->buffered) {
				fifo_put(&this->queue, scalar);
			} else {
				this->value = scalar;
			}
			++this->occupation;

			catch (condition_signal(&this->non_empty));
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static inline int
channel_receive (Channel *const this, Scalar response[static 1])
{
	if (CHANNEL_DRY & this->flags) {
		*response = Unsigned(0x0);
		return STATUS_SUCCESS;
	}

	int err;
	enter_monitor(this);

	switch (this->mode) {
		case CHANNEL_MODE_SYNC:
			catch (board_receive(this->board));
			*response = this->value;
			--this->occupation;
			break;
		case CHANNEL_MODE_ASYNC:
			auto bool non_empty(void) { 
				return this->occupation > 0;
			}
			catch (condition_await(&this->non_empty, &this->syncronized, non_empty));

			if (this->buffered) {
				*response = fifo_get(&this->queue);
			} else {
				*response = this->value;
			}
			--this->occupation;

			catch (condition_signal(&this->non_full));
			break;
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
