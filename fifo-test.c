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

	for (char c='0'; !_chn_full(&chn); ++c) {
		catch(chn_send(&chn, c));
	}

	Scalar s;
	for (char c; !_chn_empty(&chn);) {
		catch(chn_receive(&chn, &s));
		c = chn_cast(s, c);
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
