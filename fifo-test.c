// Channel test

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>

#include "channel.h"

// comment next line to enable assertions
//#define NDEBUG
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// FIFO test
////////////////////////////////////////////////////////////////////////

#define N 10

int main(int argc, char* argv[])
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) goto onerror

	Channel chn;
	catch (chn_init(&chn, N));

	for (int i=0; i < N; ++i) {
		catch (chn_send(&chn, '0'+i));
	}

	Scalar s;
	for (int i=0; i < N; ++i) {
		catch (chn_receive(&chn, &s));
		char c = chn_cast(s, '@');
		putchar(c);
	}

	putchar('\n');

	chn_destroy(&chn);

	return EXIT_SUCCESS;

onerror:
#	undef catch
	enum { internal_error };
	static const char const* ename[] = {
		[thrd_success] = "thrd_success",
		[thrd_nomem] = "thrd_nomem",
		[thrd_busy] = "thrd_busy",
		[thrd_error] = "thrd_error",
		[thrd_timedout] = "thrd_timedout",
	};
	assert(err != thrd_success);
	switch (err) {
		case thrd_nomem:
		case thrd_busy:
		case thrd_error:
		case thrd_timedout:
			error(EXIT_FAILURE, ENOMEM, "%s", ename[err]);
		default: assert(internal_error);
	}
	return EXIT_FAILURE;
}

// vim:ai:sw=4:ts=4
