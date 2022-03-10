#ifndef PORT_H
#define PORT_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../scalar.h"
#include "../monitor/lock.h"
#include "../monitor/notice.h"

////////////////////////////////////////////////////////////////////////
// Type Port of one scalar
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Port {
	unsigned pending;
	Scalar   value;
	Lock     entry;
	Notice   rendezvous[2];
} Port;

static void port_destroy(Port *const this);
static int  port_init(Port *const this);
static int  port_receive(Port *const this, Scalar* message);
static int  port_send(Port *const this, Scalar message);

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

	if ((err=(lock_init(&this->entry))) != STATUS_SUCCESS) {
		return err;
	}
	if ((err=(notice_init(this->rendezvous+0, &this->entry))) != STATUS_SUCCESS) {
		lock_destroy(&this->entry);
		return err;
	}
	if ((err=(notice_init(this->rendezvous+1, &this->entry))) != STATUS_SUCCESS) {
		notice_destroy(this->rendezvous+0);
		lock_destroy(&this->entry);
		return err;
	}
	return STATUS_SUCCESS;
}

static inline void
port_destroy (Port *const this)
{
	assert(_port_empty(this));

	notice_destroy(this->rendezvous+1);
	notice_destroy(this->rendezvous+0);
	lock_destroy(&this->entry);
}

//
// Monitor helpers
//
#define ENTER_PORT_MONITOR\
	if ((err=lock_acquire(&this->entry))!=STATUS_SUCCESS) {\
		return err;\
	}

#define LEAVE_PORT_MONITOR\
	if ((err=lock_release(&this->entry))!=STATUS_SUCCESS) {\
		return err;\
	}

static inline int
port_send (Port *const this, Scalar message)
{
#   define catch(X) if ((err=(X)) != STATUS_SUCCESS) goto onerror
	int err;
	ENTER_PORT_MONITOR

	// protocol
	//    thread a: check(0)-A-notify(1)
	//    thread b: notify(0)-check(1)-B
	//
	catch (notice_check(this->rendezvous+0));
	this->value = message;
	catch (notice_notify(this->rendezvous+1));

	++this->pending;

	LEAVE_PORT_MONITOR

	return STATUS_SUCCESS;
onerror:
#   undef catch
	lock_release(&this->entry);
	return err;
}

static inline int
port_receive (Port *const this, Scalar* message)
{
#   define catch(X) if ((err=(X)) != STATUS_SUCCESS) goto onerror
	int err;
	ENTER_PORT_MONITOR

	// protocol
	//    thread a: check(0)-A-notify(1)
	//    thread b: notify(0)-check(1)-B
	//
	catch (notice_notify(this->rendezvous+0));
	catch (notice_check(this->rendezvous+1));
	if (message) *message = this->value;

	--this->pending;

	LEAVE_PORT_MONITOR

	return STATUS_SUCCESS;
onerror:
#   undef catch
	lock_release(&this->entry);
	return err;
}

#undef ENTER_PORT_MONITOR
#undef LEAVE_PORT_MONITOR

#endif // vim:ai:sw=4:ts=4:syntax=cpp
