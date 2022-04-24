#ifndef MONITOR_H
#define MONITOR_H

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

#endif // vim:ai:sw=4:ts=4:syntax=cpp
