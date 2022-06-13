#ifndef POLY_ENTRY_H
#define POLY_ENTRY_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#ifndef POLY_PASS_H
#include "PASS.h"
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

static int  entry_accept(Entry *const this, void(thunk)(void));
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
	MONITOR_ENTRY
	bool const r = notice_ready(&this->board[0]);
	ENTRY_END
	return r;
}

static int
entry_call (Entry *const this, Scalar request, Scalar response[static 1])
{
	MONITOR_ENTRY

	auto void thunk(void) {
		this->request = request;
	}
	catch (board_call(this->board, thunk));
	response[0] = this->response;
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

////////////////////////////////////////////////////////////////////////
// Rendezvous
////////////////////////////////////////////////////////////////////////

#define INTERFACE_TYPE(T)   struct POLY_PACKED T##_face {

#define INTERFACE_SLOT(T)   struct T##_face * I_

#define interface(T)        struct T##_face

/*
 *
 *  INTERFACE_TYPE (T)
 *      Entry e1;
 *      Entry e2;
 *      ...
 *  END_TYPE
 *
 *  THREAD_TYPE (T)
 *      ...
 *      INTERFACE_SLOT (T);
 *  END_TYPE
 *
 *  static interface(Printer) IPrinter;
 *  interface_init(n, &IPrinter);
 *  err = go_task(Printer,...);
 *  Scalar r;
 *  entry_call(&IPrinter.e1, s, &r);
 */

#define select          int _open=0; int _selec=0;

#define entry(E)        &this.I_->E

#define when(G,E)       if ((G) && ++_open && entry_ready(entry(E)) && ++_selec)

#define terminate       if (!_open) panic("ops!") elif (!_selec) break else continue
 
/*
 *
 *  for (;;) {
 *      select {
 *          when (guard, e1) {
 *              void thunk(void) {
 *                  Scalar s = f(entry(e1)->request);
 *                  entry(e1)->response = s;
 *              }
 *              catch (entry_accept(entry(e1), thunk));
 *          }
 *    //or
 *          when ...
 *    //or
 *          when ...
 *    //or
 *          terminate;
 *      }
 *  }
 */

#undef ASSERT_ENTRY_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
