/*
 * Rendez-Vous
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#ifndef POLY_H
#include "POLY.h"
#endif
#include "lock.h"
#include "event.h"

////////////////////////////////////////////////////////////////////////
// Type RendezVous
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct RendezVous {
	Event pair[2];
} RendezVous;

static inline int  rv_init(RendezVous* self, union lck_ptr lock);
static inline void rv_destroy(RendezVous* self);
static inline int  rv_wait(RendezVous* self, int i);
static inline int  rv_signal(RendezVous* self, int i);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int rv_init(RendezVous* self, union lck_ptr lock)
{
	int err;
	if ((err=evt_init(&self->pair[0], lock.mutex)) == STATUS_SUCCESS) {
		if ((err=evt_init(&self->pair[1], lock.mutex)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			evt_destroy(&self->pair[0]);
		}
	}
	return err;
}

static inline void rv_destroy(RendezVous* self)
{
	evt_destroy(&self->pair[1]);
	evt_destroy(&self->pair[0]);
}

static ALWAYS inline int rv_wait(RendezVous* self, int i)
{
	assert(i==0 || i==1);
	return evt_wait(&self->pair[i]);
}

static ALWAYS inline int rv_signal(RendezVous* self, int i)
{
	assert(i==0 || i==1);
	return evt_signal(&self->pair[i]);
}

#endif // RENDEZVOUS_H

// vim:ai:sw=4:ts=4:syntax=cpp
