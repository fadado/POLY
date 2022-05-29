#ifndef POLY_MONITOR_H
#define POLY_MONITOR_H

////////////////////////////////////////////////////////////////////////
// Monitor helpers
////////////////////////////////////////////////////////////////////////

/*
 * Define:
 *     + auto int err;
 * Assume:
 *     + `syncronized` is the lock
 */

#define MONITOR_ENTRY                                            \
	int err;                                                     \
	if ((err=lock_acquire(&this->syncronized))!=STATUS_SUCCESS){ \
		return err;                                              \
	}

#define ENTRY_END                                                \
	if ((err=lock_release(&this->syncronized))!=STATUS_SUCCESS){ \
		return err;                                              \
	}                                                            \
	return STATUS_SUCCESS;                                       \
onerror:                                                         \
	lock_release(&this->syncronized);                            \
	return err;

#endif // vim:ai:sw=4:ts=4:syntax=cpp
