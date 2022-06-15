/* printer3.c */

#include <stdio.h>
#include "poly/thread.h"
#include "poly/passing/entry.h"
#include "poly/passing/interface.h"

INTERFACE_TYPE(Printer3)
	Entry print;
END_TYPE

THREAD_TYPE(Printer3)
	int i;
	INTERFACE_SLOT(Printer3);
END_TYPE

THREAD_BODY(Printer3)
	void thunk(void) {
		char* msg = cast(entry(print).request, char*);
		if (msg != NULL && *msg != '\0') {
			printf("%d: %s\n", this.i, msg);
		}
		entry(print).response = Signed(this.i);
	}
	entry_accept(&entry(print), thunk);
END_BODY

int main()
{
	int err;
	interface(Printer3) iface1, iface2;

	catch (interface_init(ENTRIES(iface1), &iface1));
	catch (interface_init(ENTRIES(iface2), &iface2));

	// TODO: bug???
#if 1 // OK
	err = run_task(Printer3, &iface1, .i=1);
	catch (err);
#elif 0 // OK
	err = thread_create(&(Thread){0},
			Printer3_body,
			&(struct Printer3_type){.i=1, .IFACE_=&iface1});
	catch (err);
#elif 0
	if ((err=thread_create(&(Thread){0},
			Printer3_body,
			&(struct Printer3_type){.i=1, .IFACE_=&iface1})) != 0)
		goto onerror;
#elif 0
	if (thread_create(&(Thread){0},
			Printer3_body,
			&(struct Printer3_type){.i=1, .IFACE_=&iface1}) != 0)
		goto onerror;
#elif 0
	// !!!!!!!!!!!!!!!!!
	struct Printer3_type* p = &(struct Printer3_type){.i=1, .IFACE_=&iface1};
	if (thread_create(&(Thread){0}, Printer3_body, p) != 0)
		goto onerror;
#elif 1
	catch (run_task(Printer3, &iface1, .i=1));
#endif
	err = run_task(Printer3, &iface2, .i=2);
	catch (err);

	Scalar s, r;
	s = Pointer("Good Morning!");
	catch (entry_call(&iface1.print, s, &r));
	assert(cast(r, int) == 1);
	catch (entry_call(&iface2.print, s, &r));
	assert(cast(r, int) == 2);

	interface_destroy(ENTRIES(iface1), &iface1);
	interface_destroy(ENTRIES(iface1), &iface2);

	return 0;
onerror:
	char* msg;
	switch (err) {
		case STATUS_BUSY: msg="BUSY"; break;
		case STATUS_ERROR: msg="ERROR"; break;
		case STATUS_NOMEM: msg="NOMEM"; break;
		case STATUS_TIMEDOUT: msg="TIMEDOUT"; break;
		default: msg="unknown error code"; break;
	}
	error(1, 0, "%s (%d)", msg, err);
}

// vim:ai:sw=4:ts=4:syntax=cpp
