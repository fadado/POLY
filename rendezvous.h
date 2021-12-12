/*
 * Rendez-Vous
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

#include "event.h" // include <thread.h>

////////////////////////////////////////////////////////////////////////
// Type RendezVous
// Interface
////////////////////////////////////////////////////////////////////////

typedef Event RendezVous[2];

#ifdef DEBUG
const char* _RV_[2] = {
	[0] = "HOLA_DON_PEPITO",
	[1] = "HOLA_DON_JOSE",
};
#endif

static inline int  rv_init(RendezVous self);
static inline void rv_destroy(RendezVous self);
static inline int  rv_wait(RendezVous self, int i, mtx_t* mutex);
static inline int  rv_signal(RendezVous self, int i);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int rv_init(RendezVous self)
{
	int err;
	if ((err=evt_init(&self[0])) == thrd_success) {
		if ((err=evt_init(&self[1])) == thrd_success) {
			return thrd_success;
		} else {
			evt_destroy(&self[0]);
		}
	}
	return err;
}

static inline void rv_destroy(RendezVous self)
{
	evt_destroy(&self[0]);
	evt_destroy(&self[1]);
}

static inline int rv_wait(RendezVous self, int i, mtx_t* mutex)
{
	assert(i==0 || i==1);

	trace("WAIT   @ %s", _RV_[i]);

	return evt_wait(&self[i], mutex);
}

static inline int rv_signal(RendezVous self, int i)
{
	assert(i==0 || i==1);

	trace("SIGNAL @ %s", _RV_[i]);

	return evt_signal(&self[i]);
}

#endif // RENDEZVOUS_H

// vim:ai:sw=4:ts=4:syntax=cpp
