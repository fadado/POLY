// Channel test

#include <errno.h>	// ENOMEM
#include <string.h>	// strerror
#include <stdio.h>	// IO
#include <stdlib.h>	// abort

#include "channel.h" // stdbool, stdint, stdlib, threads

#define N 10

int main(int argc, char* argv[])
{
	int err;
#	define catch(e)	if ((err=(e)) != thrd_success) goto onerror

	Channel chn;
	catch(chn_init(&chn, N));

	for (int i=0; i < N; ++i) {
		catch(chn_send(&chn, '0'+i));
	}

	Scalar s;
	for (int i=0; i < N; ++i) {
		catch(chn_receive(&chn, &s));
		char c = chn_cast(s, i);
		putchar(c);
	}

	putchar('\n');

	chn_destroy(&chn);

	return EXIT_SUCCESS;

onerror:
#	undef catch
	switch (err) {
		case thrd_nomem:
			fprintf(stderr, "%s\n", strerror(ENOMEM));
			break;
		case thrd_busy: // TODO
		case thrd_error:
		case thrd_timedout:
		default: abort();
	}
	return EXIT_FAILURE;
}

// vim:ai:sw=4:ts=4
