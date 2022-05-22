#ifndef POLY_PORT_H
#define POLY_PORT_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"
#include "../scalar.h"

////////////////////////////////////////////////////////////////////////
// Port interface
////////////////////////////////////////////////////////////////////////

typedef struct Port {
	Lock    syncronized;
	Notice  board[2];
	Scalar  value;
} Port;

static void port_destroy(Port *const this);
static int  port_init(Port *const this);
static int  port_receive(Port *const this, Scalar* scalar);
static bool port_ready(Port const*const this);
static int  port_send(Port *const this, Scalar scalar);

////////////////////////////////////////////////////////////////////////
// Port implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_PORT_INVARIANT
#else
#	define ASSERT_PORT_INVARIANT
#endif

static int
port_init (Port *const this)
{
	int err;

	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(board_init(2, this->board, &this->syncronized))) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
	ASSERT_PORT_INVARIANT

	return STATUS_SUCCESS;
}

static void
port_destroy (Port *const this)
{
	board_destroy(2, this->board);
	lock_destroy(&this->syncronized);
}

static ALWAYS inline bool
port_ready (Port const*const this)
{
	return notice_ready(&this->board[0]);
}

static int
port_send (Port *const this, Scalar scalar)
{
	int err;
	enter_monitor(this);

	auto void thunk(void) {
		this->value = scalar;
	}
	catch (board_send(this->board, thunk));
	ASSERT_PORT_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static int
port_receive (Port *const this, Scalar* scalar)
{
	int err;
	enter_monitor(this);

	catch (board_receive(this->board));
	if (scalar != NULL) {
		*scalar = this->value;
	}
	ASSERT_PORT_INVARIANT

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#undef ASSERT_PORT_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
