#ifndef POLY_MONITOR_H
#define POLY_MONITOR_H

////////////////////////////////////////////////////////////////////////
// Monitor helpers
////////////////////////////////////////////////////////////////////////

/*
 * Assume:
 *     + auto int err;
 *     + `syncronized` is the lock
 */

#define enter_monitor(OBJ) do {\
	if ((err=lock_acquire(&(OBJ)->syncronized))!=STATUS_SUCCESS){\
		return err;\
	}\
} while (0)

#define leave_monitor(OBJ) do {\
	if ((err=lock_release(&(OBJ)->syncronized))!=STATUS_SUCCESS){\
		return err;\
	}\
} while (0)

#define break_monitor(OBJ)\
	lock_release(&(OBJ)->syncronized)

////////////////////////////////////////////////////////////////////////

#define MONITOR_ENTRY\
	int err;\
	if ((err=lock_acquire(&this->syncronized))!=STATUS_SUCCESS){\
		return err;\
	}

#define ENTRY_END\
	if ((err=lock_release(&this->syncronized))!=STATUS_SUCCESS){\
		return err;\
	}\
	return STATUS_SUCCESS;\
onerror:\
	lock_release(&this->syncronized);\
	return err;

#endif // vim:ai:sw=4:ts=4:syntax=cpp
