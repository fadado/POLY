#ifndef POLY_ENTRY_H
#define POLY_ENTRY_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"
#include "../scalar.h"

////////////////////////////////////////////////////////////////////////
// Entry interface
////////////////////////////////////////////////////////////////////////

typedef struct Entry {
	Lock    syncronized;
	Notice  board[3];
	Scalar  query;
	Scalar  reply;
} Entry;

static int  entry_accept(Entry *const this, void(thunk)(void));
static int  entry_call(Entry *const this, Scalar query, Scalar reply[static 1]);
static void entry_destroy(Entry *const this);
static int  entry_init(Entry *const this);
static bool entry_ready(Entry *const this);

////////////////////////////////////////////////////////////////////////
// Entry implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_ENTRY_INVARIANT
#else
#	define ASSERT_ENTRY_INVARIANT
#endif

static int
entry_init (Entry *const this)
{
	int err;

	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(board_init(3, this->board, &this->syncronized))) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_ENTRY_INVARIANT

	return STATUS_SUCCESS;
}

static void
entry_destroy (Entry *const this)
{
	board_destroy(3, this->board);
	lock_destroy(&this->syncronized);
}

////////////////////////////////////////////////////////////////////////

static ALWAYS inline bool
entry_ready (Entry *const this)
{
    lock_acquire(&this->syncronized);
	bool const r = notice_ready(&this->board[0]);
    lock_release(&this->syncronized);
	return r;
}

static int
entry_call (Entry *const this, Scalar query, Scalar reply[static 1])
{
	MONITOR_ENTRY

	auto void thunk(void) {
		this->query = query;
	}
	catch (board_call(this->board, thunk));
	reply[0] = this->reply;
	ASSERT_ENTRY_INVARIANT

	ENTRY_END
}

static int
entry_accept (Entry *const this, void(thunk)(void))
{
	MONITOR_ENTRY

	catch (board_accept(this->board, thunk));
	ASSERT_ENTRY_INVARIANT

	ENTRY_END
}

#undef ASSERT_ENTRY_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
