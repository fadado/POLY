#ifndef PORT_H
#define PORT_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../scalar.h"
#include "../monitor/lock.h"
#include "../monitor/notice.h"
#include "../monitor/board.h"

////////////////////////////////////////////////////////////////////////
// Type Port of one scalar
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Port {
	unsigned pending;
	Scalar   value;
	Lock     monitor;
	Notice   board[2];
} Port;

static void port_destroy(Port *const this);
static int  port_init(Port *const this);
static int  port_receive(Port *const this, Scalar* request);
static int  port_send(Port *const this, Scalar scalar);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

//
// Predicates
//
static ALWAYS inline bool
_port_empty (Port const*const this)
{
	return this->pending == 0;
}

static ALWAYS inline bool
_port_full (Port const*const this)
{
	return this->pending == 1;
}

//
// Port life
//

static int
port_init (Port *const this)
{
	int err;

	this->pending = 0;

	if ((err=(lock_init(&this->monitor))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(board_init(this->board, 2, &this->monitor))) != STATUS_SUCCESS) {
		lock_destroy(&this->monitor);
		return err;
	}
	return STATUS_SUCCESS;
}

static void
port_destroy (Port *const this)
{
	assert(_port_empty(this));

	board_destroy(this->board, 2);
	lock_destroy(&this->monitor);
}

//
// Monitor helpers
//
#define ENTER_PORT_MONITOR\
	if ((err=lock_acquire(&this->monitor))!=STATUS_SUCCESS) {\
		return err;\
	}

#define LEAVE_PORT_MONITOR\
	if ((err=lock_release(&this->monitor))!=STATUS_SUCCESS) {\
		return err;\
	}

#define catch(X)\
	if ((err=(X)) != STATUS_SUCCESS) {\
		lock_release(&this->monitor);\
		return err;\
	}

static inline int
port_send (Port *const this, Scalar scalar)
{
	int err;
	ENTER_PORT_MONITOR

	void thunk(void) {
		this->value = scalar;
	}
	catch (board_send(this->board, thunk));
	++this->pending;

	LEAVE_PORT_MONITOR

	return STATUS_SUCCESS;
}

static inline int
port_receive (Port *const this, Scalar* request)
{
	int err;
	ENTER_PORT_MONITOR

	catch (board_receive(this->board));
	if (request) *request = this->value;
	--this->pending;

	LEAVE_PORT_MONITOR

	return STATUS_SUCCESS;
}

#undef catch
#undef ENTER_PORT_MONITOR
#undef LEAVE_PORT_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
