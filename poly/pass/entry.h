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
	Scalar  request;
	Scalar  response;
} Entry;

static int  entry_accept(Entry *const this, void(accept)(void));
static int  entry_call(Entry *const this, Scalar request, Scalar response[static 1]);
static void entry_destroy(Entry *const this);
static int  entry_init(Entry *const this);
static bool entry_ready(Entry const*const this);

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
entry_ready (Entry const*const this)
{
	return notice_ready(&this->board[0]);
}

////////////////////////////////////////////////////////////////////////

static int
entry_call (Entry *const this, Scalar request, Scalar response[static 1])
{
	MONITOR_ENTRY

	auto void thunk(void) {
		this->request = request;
	}
	catch (board_call(this->board, thunk))
	*response = this->response;
	ASSERT_ENTRY_INVARIANT

	ENTRY_END
}

static int
entry_accept (Entry *const this, void(accept)(void))
{
	MONITOR_ENTRY

	catch (board_accept(this->board, accept))
	ASSERT_ENTRY_INVARIANT

	ENTRY_END
}

////////////////////////////////////////////////////////////////////////
// Server tasks experiments
////////////////////////////////////////////////////////////////////////

/*
 *  TASK_TYPE (Server)
 *      Entry* entry1;
 *      Entry* entry2;
 *      ...
 *  END_TYPE
 *
 *  Scalar s=x, r;
 *  call (&entry, s, &r);
 *
 *  #define call(e,s,r) catch (entry_call(e, s, r))
 *  #define loop        for (;;)
 *  #define select      int _open=0; int _selec=0;
 *  #define when(g,e)   if ((g) && ++_open && entry_ready(e) && ++_selec)
 *  #define terminate   if (!_open) panic("ops") elif (!_selec) break else continue
 *  #define or
 *
 *  loop {
 *      select {
 *          when (guard, this.entry) {
 *              void accept(void) {
 *                  ...
 *                  this.entry->response = ϕ(this.entry->request);
 *                  ...
 *              }
 *              catch (entry_accept(this.entry, accept))
 *          }
 *      or
 *          when ...
 *      or
 *          when ...
 *      or
 *          terminate;
 *      }
 *  }
 */

#undef ASSERT_ENTRY_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
