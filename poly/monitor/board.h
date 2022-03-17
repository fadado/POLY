#ifndef BOARD_H
#define BOARD_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "lock.h"
#include "notice.h"

////////////////////////////////////////////////////////////////////////
// Type Board of 2 or 3 Notice
// Interface
////////////////////////////////////////////////////////////////////////

static int  board_init(Notice board[], unsigned n, union Lock lock);
static void board_destroy(Notice board[], unsigned n);

static int  board_notify(Notice board[], unsigned i);
static int  board_enquire(Notice board[], unsigned i);

static int  board_meet(Notice board[2], unsigned i);

static int  board_send(Notice board[2], void(thunk)(void));
static int  board_receive(Notice board[2]);

static int  board_call(Notice board[3], void(thunk)(void));
static int  board_accept(Notice board[3], void(thunk)(void));

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

// Same error management strategy in all this module
#define catch(X)\
	if ((err=(X)) != STATUS_SUCCESS)\
		return err

static int
board_init (Notice board[], unsigned n, union Lock lock)
{
	assert(n > 0);
	for (unsigned i = 0; i < n; ++i) {
		const int err = notice_init(&board[i], lock.mutex);
		if (err != STATUS_SUCCESS) {
			if (i > 0) {
				board_destroy(board, i);
			}
			return err;
		}
	}
	return STATUS_SUCCESS;
}

static void
board_destroy (Notice board[], unsigned n)
{
	assert(n > 0);
	unsigned i = n;
	do {
		--i;
		notice_destroy(&board[i]);
	} while (i != 0);
}

static ALWAYS inline int
board_enquire (Notice board[], unsigned i)
{
	return notice_enquire(&board[i]);
}

static ALWAYS inline int
board_notify (Notice board[], unsigned i)
{
	return notice_notify(&board[i]);
}

//
// operations on 2 elements board
//

static ALWAYS inline int
board_meet (Notice board[2], unsigned i)
{
	// Thread A | Thread B
	// ---------+---------
	// meet(0)  |  meet(1)
	assert(i < 2);
	int err;
	catch (board_notify(board, i));
	catch (board_enquire(board, !i));
	return STATUS_SUCCESS;
}

static ALWAYS inline int
board_receive (Notice board[2])
{
	return board_meet(board, 0);
}

static ALWAYS inline int
board_send (Notice board[2], void(thunk)(void))
{
	int err;
	catch (board_enquire(board, 0));
	thunk();
	catch (board_notify(board, 1));
	return STATUS_SUCCESS;
}

//
// operations on 3 elements board
//

static ALWAYS inline int
board_call (Notice board[3], void(thunk)(void))
{
	int err;
	catch (board_enquire(board, 0));
	thunk();
	catch (board_notify(board, 1));
	catch (board_enquire(board, 2));
	return STATUS_SUCCESS;
}

static ALWAYS inline int
board_accept (Notice board[3], void(thunk)(void))
{
	int err;
	catch (board_notify(board, 0));
	catch (board_enquire(board, 1));
	thunk();
	catch (board_notify(board, 2));
	return STATUS_SUCCESS;
}

#undef catch

#endif // vim:ai:sw=4:ts=4:syntax=cpp
