#ifndef POLY_HANDSHAKE_H
#define POLY_HANDSHAKE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"

////////////////////////////////////////////////////////////////////////
// Handshake interface
////////////////////////////////////////////////////////////////////////

typedef struct Handshake {
	Lock      syncronized;
	Notice    board[2];
	unsigned  who; // alternates between 0 and 1
} Handshake;

static int  handshake_init(Handshake *const this);
static void handshake_destroy(Handshake *const this);
static int  handshake_wait(Handshake *const this);

////////////////////////////////////////////////////////////////////////
// Handshake implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_HANDSHAKE_INVARIANT\
		assert(this->who < 2);
#else
#	define ASSERT_HANDSHAKE_INVARIANT
#endif

/*  Handshake h;
 *
 *  catch (handshake_init(&b));
 *  ...
 *  handshake_destroy(&b);
 */

static int
handshake_init (Handshake *const this)
{
	this->who = 0;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(board_init(2, this->board, &this->syncronized))) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_HANDSHAKE_INVARIANT

	return STATUS_SUCCESS;
}

static void
handshake_destroy (Handshake *const this)
{
	board_destroy(2, this->board);
	lock_destroy(&this->syncronized);
}

/*
 * catch (handshake_wait(&b)) | catch (handshake_wait(&b))
 */

static inline int
handshake_wait (Handshake *const this)
{
	int err;
	enter_monitor(this);

	unsigned const who = this->who;
	this->who = !who;
	catch (board_meet(this->board, who));
	ASSERT_HANDSHAKE_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#undef ASSERT_HANDSHAKE_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
