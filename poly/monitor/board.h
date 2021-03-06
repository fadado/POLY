#ifndef POLY_BOARD_H
#define POLY_BOARD_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#ifndef POLY_MONITOR_H
#include "MONITOR.h"
#endif
#include "lock.h"
#include "notice.h"

/*
 * Boards are arrays of Notice, handy in the implementation of rendezvous
 * protocols.
 */

////////////////////////////////////////////////////////////////////////
// Board interface
////////////////////////////////////////////////////////////////////////

static int  board_init(unsigned n, Notice board[static n], union Lock lock);
static void board_destroy(unsigned n, Notice board[static n]);

static int  board_meet(Notice board[static 2], unsigned i);

static int  board_send(Notice board[static 2], void(thunk)(void));
static int  board_receive(Notice board[static 2]);

static int  board_call(Notice board[static 3], void(thunk)(void));
static int  board_accept(Notice board[static 3], void(thunk)(void));

////////////////////////////////////////////////////////////////////////
// Board implementation
////////////////////////////////////////////////////////////////////////

static int
board_init (unsigned n, Notice board[static n], union Lock lock)
{
	assert(n > 0);
	for (unsigned i = 0; i < n; ++i) {
		const int err = notice_init(&board[i], lock);
		if (err != STATUS_SUCCESS) {
			if (i > 0) {
				board_destroy(i, board);
			}
			return err;
		}
	}
	return STATUS_SUCCESS;
}

static void
board_destroy (unsigned n, Notice board[static n])
{
	assert(n > 0);
	do {
		--n;
		notice_destroy(&board[n]);
	} while (n != 0);
}

/*
 * Operations on 2 elements board.
 */

static ALWAYS inline int
board_meet (Notice board[static 2], unsigned i)
{
	// Thread A | Thread B
	// ---------+---------
	// meet(0)  |  meet(1)

	assert(i < 2);
	int err;

	catch (notice_signal(&board[i]));
	catch (notice_wait(&board[!i]));

	return STATUS_SUCCESS;
onerror:
	return err;
}

static ALWAYS inline int
board_send (Notice board[static 2], void(thunk)(void))
{
	int err;

	catch (notice_wait(&board[0]));
	thunk();
	catch (notice_signal(&board[1]));

	return STATUS_SUCCESS;
onerror:
	return err;
}

static ALWAYS inline int
board_receive (Notice board[static 2])
{
	int err;

	catch (notice_signal(&board[0]));
	catch (notice_wait(&board[1]));

	return STATUS_SUCCESS;
onerror:
	return err;
}

/*
 * Operations on 3 elements board.
 */

static ALWAYS inline int
board_call (Notice board[static 3], void(thunk)(void))
{
	int err;

	catch (notice_wait(&board[0]));
	thunk();
	catch (notice_signal(&board[1]));
	catch (notice_wait(&board[2]));

	return STATUS_SUCCESS;
onerror:
	return err;
}

static ALWAYS inline int
board_accept (Notice board[static 3], void(thunk)(void))
{
	int err;

	catch (notice_signal(&board[0]));
	catch (notice_wait(&board[1]));
	thunk();
	catch (notice_signal(&board[2]));

	return STATUS_SUCCESS;
onerror:
	return err;
}

#endif // vim:ai:sw=4:ts=4:syntax=cpp
