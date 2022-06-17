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
END_TYPE

THREAD_BODY(Printer3)
	void accept(void) {
		char* msg = cast(entry(print).query, char*);
		if (msg != NULL && *msg != '\0') {
			printf("%d: %s\n", this.i, msg);
		}
		entry(print).reply = Signed(this.i);
	}
	entry_accept(&entry(print), accept);
END_BODY

int main()
{
	int err;
	interface(Printer3) iface1, iface2;

	catch (interface_init(ENTRIES(iface1), &iface1));
	catch (interface_init(ENTRIES(iface2), &iface2));

	run_task(Printer3, &iface1, .i=1);
	run_task(Printer3, &iface2, .i=2);

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
