#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Handshake {
	Lock     monitor;
	Notice   board[2];
	unsigned who;
} Handshake;

static int  handshake_init(Handshake *const this);
static void handshake_destroy(Handshake *const this);
static int  handshake_wait(Handshake *const this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_HANDSHAKE_INVARIANT\
		assert(this->who < 2);
#else
#	define ASSERT_HANDSHAKE_INVARIANT
#endif

static int
handshake_init (Handshake *const this)
{
	this->who = 0;

	int err;
	if ((err=(lock_init(&this->monitor))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(board_init(this->board, 2, &this->monitor))) != STATUS_SUCCESS) {
		lock_destroy(&this->monitor);
		return err;
	}
	ASSERT_HANDSHAKE_INVARIANT

	return STATUS_SUCCESS;
}

static void
handshake_destroy (Handshake *const this)
{
	board_destroy(this->board, 2);
	lock_destroy(&this->monitor);
}

//
//Monitor helpers
//
#define ENTER_MONITOR\
	if ((err=lock_acquire(&this->monitor))!=STATUS_SUCCESS){\
		return err;\
	}

#define LEAVE_MONITOR\
	if ((err=lock_release(&this->monitor))!=STATUS_SUCCESS){\
		return err;\
	}

static inline int
handshake_wait (Handshake *const this)
{
	int err;
	ENTER_MONITOR

	unsigned const who = this->who;
	this->who = !who;
	catch (board_meet(this->board, who));
	ASSERT_HANDSHAKE_INVARIANT

	LEAVE_MONITOR
	return STATUS_SUCCESS;
onerror:
	lock_release(&this->monitor);
	return err;
}

#undef ASSERT_HANDSHAKE_INVARIANT
#undef ENTER_MONITOR
#undef LEAVE_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
