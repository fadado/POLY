#ifndef ENTRY_H
#define ENTRY_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"
#include "scalar.h"

////////////////////////////////////////////////////////////////////////
// Type Entry
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Entry {
	Lock     syncronized;
	Notice   board[3];
	Scalar   request;
	Scalar   response;
} Entry;

static int  entry_accept(Entry *const this, void(accept)(void));
static int  entry_call(Entry *const this, Scalar request, Scalar* response);
static void entry_destroy(Entry *const this);
static int  entry_init(Entry *const this);
static bool entry_ready(Entry const*const this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_ENTRY_INVARIANT
#else
#	define ASSERT_ENTRY_INVARIANT
#endif

//
// Predicates
//
static ALWAYS inline bool
entry_ready (Entry const*const this)
{
	return notice_ready(&this->board[0]);
}

//
// Entry life
//

static int
entry_init (Entry *const this)
{
	int err;

	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(board_init(this->board, 3, &this->syncronized))) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_ENTRY_INVARIANT

	return STATUS_SUCCESS;
}

static void
entry_destroy (Entry *const this)
{
	board_destroy(this->board, 3);
	lock_destroy(&this->syncronized);
}

static inline int
entry_call (Entry *const this, Scalar request, Scalar* response)
{
	int err;
	enter_monitor(this);

	void thunk(void) {
		this->request = request;
	}
	catch (board_call(this->board, thunk));
	if (response != NULL) {
		*response = this->response;
	}
	ASSERT_ENTRY_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static inline int
entry_accept (Entry *const this, void(accept)(void))
{
	int err;
	enter_monitor(this);

	catch (board_accept(this->board, accept));
	ASSERT_ENTRY_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#undef ASSERT_ENTRY_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
