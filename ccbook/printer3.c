/* printer3.c */

#include <stdio.h>
#include "poly/thread.h"
#include "poly/passing/entry.h"
#include "poly/passing/task.h"

typedef struct {
	Entry print;
} _Printer3;

struct Printer3 {
	TASK_TYPE (_Printer3)
	int i;
};

int Printer3(void* data)
{
	THREAD_BODY (Printer3, data)

	void accept(void) {
		char* msg = cast(entry(print).query, char*);
		if (msg != NULL && *msg != '\0') {
			printf("%d: %s\n", this.i, msg);
		}
		entry(print).reply = Signed(this.i);
	}
	entry_accept(&entry(print), accept);

	END_BODY
}

int main()
{
	int err;

	_Printer3 printer[2];
	catch (task_init(ENTRIES(_Printer3), &printer[0]));
	catch (task_init(ENTRIES(_Printer3), &printer[1]));

	run_task(Printer3, &printer[0], .i=1);
	run_task(Printer3, &printer[1], .i=2);

	Scalar s, r;
	s = Pointer("Good Morning!");
	catch (entry_call(&printer[0].print, s, &r));
	assert(cast(r, int) == 1);
	catch (entry_call(&printer[1].print, s, &r));
	assert(cast(r, int) == 2);

	task_destroy(ENTRIES(_Printer3), &printer[0]);
	task_destroy(ENTRIES(_Printer3), &printer[1]);

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
