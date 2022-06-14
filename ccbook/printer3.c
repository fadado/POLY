/* printer3.c */

#include <stdio.h>
#include "poly/thread.h"
#include "poly/passing/entry.h"
#include "poly/passing/interface.h"

static atomic(int) done;

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
		entry(print).response = Signed(0);
	}
	entry_accept(&entry(print), thunk);
	++done;
END_BODY

int main()
{
	interface(Printer3) iface1, iface2;
	interface_init(1, &iface1);
	interface_init(1, &iface2);

	run_task(Printer3, &iface1, .i=1);
	run_task(Printer3, &iface2, .i=2);

	Scalar s, r;
	s = Pointer("Good Morning!");
	entry_call(&iface1.print, s, &r);
	entry_call(&iface2.print, s, &r);

	while (done < 2) { thread_yield(); }

	interface_destroy(1, &iface1);
	interface_destroy(1, &iface2);
}

// vim:ai:sw=4:ts=4:syntax=cpp
