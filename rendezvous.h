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

static inline void rv_destroy(RendezVous* this);
static inline int  rv_init(RendezVous* this, union lck_ptr lock);
static inline int  rv_signal(RendezVous* this, int i);
static inline int  rv_wait(RendezVous* this, int i);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static inline int rv_init(RendezVous* this, union lck_ptr lock)
{
	int err;
	if ((err=evt_init(&this->pair[0], lock.mutex)) == STATUS_SUCCESS) {
		if ((err=evt_init(&this->pair[1], lock.mutex)) == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		} else {
			evt_destroy(&this->pair[0]);
		}
	}
	return err;
}

static inline void rv_destroy(RendezVous* this)
{
	evt_destroy(&this->pair[1]);
	evt_destroy(&this->pair[0]);
}

static ALWAYS inline int rv_wait(RendezVous* this, int i)
{
	assert(i==0 || i==1);
	return evt_wait(&this->pair[i]);
}

static ALWAYS inline int rv_signal(RendezVous* this, int i)
{
	assert(i==0 || i==1);
	return evt_signal(&this->pair[i]);
}

#endif // RENDEZVOUS_H

// vim:ai:sw=4:ts=4:syntax=cpp
