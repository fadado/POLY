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

static int  board_accept(Notice board[2], void(*thunk)(void));
static int  board_call(Notice board[2], void(*thunk)(void));
static int  board_enquire(Notice board[2], unsigned i);
static int  board_init(Notice board[2], union Lock lock);
static int  board_meet(Notice board[2], unsigned i);
static int  board_notify(Notice board[2], unsigned i);
static int  board_receive(Notice board[2]);
static int  board_send(Notice board[2], void(*thunk)(void));
static void board_destroy(Notice board[2]);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

// Same error management strategy in all this module
#define catch(X) if ((err=(X)) != STATUS_SUCCESS) return err

static int
board_init (Notice board[2], union Lock lock)
{
	int err;
	if ((err=(notice_init(&board[0], lock.mutex))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(notice_init(&board[1], lock.mutex))) != STATUS_SUCCESS) {
		notice_destroy(&board[0]);
		return err;
	}
	return STATUS_SUCCESS;
}

static void
board_destroy (Notice board[2])
{
	notice_destroy(&board[1]);
	notice_destroy(&board[0]);
}

static ALWAYS inline int
board_enquire (Notice board[2], unsigned i)
{
	return notice_enquire(&board[i]);
}

static ALWAYS inline int
board_notify (Notice board[2], unsigned i)
{
	return notice_notify(&board[i]);
}

// Thread A | Thread B
// ---------+---------
// meet(0)  |  meet(1)
static ALWAYS inline int
board_meet (Notice board[2], unsigned i)
{
	int err;
	catch (board_notify(board, i));
	catch (board_enquire(board, !i));
	return STATUS_SUCCESS;
}

//
static ALWAYS inline int
board_receive (Notice board[2])
{
	return board_meet(board, 0);
}

static ALWAYS inline int
board_send (Notice board[2], void(*thunk)(void))
{
	int err;
	catch (board_enquire(board, 0));
	thunk();
	catch (board_notify(board, 1));
	return STATUS_SUCCESS;
}

static ALWAYS inline int
board_call (Notice board[2], void(*thunk)(void))
{
	int err;
	catch (board_enquire(board, 0));
	thunk();
	catch (board_notify(board, 1));
	catch (board_enquire(board, 2));
	return STATUS_SUCCESS;
}

static ALWAYS inline int
board_accept (Notice board[2], void(*thunk)(void))
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
