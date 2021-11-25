// Channel test

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <threads.h>

#include "channel.h"

// comment next line to enable assertions
//#define NDEBUG
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// FIFO test
////////////////////////////////////////////////////////////////////////

#define N 10

static int producer(void* args)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) return err

	Channel* cp = args;
	for (int i=0; i < N; ++i) {
		catch (chn_send(cp, '0'+i));
	}
	return thrd_success;
#	undef catch
}

static int consumer(void* args)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) return err

	Channel* cp = args;
	Scalar s;
	for (int i=0; i < N; ++i) {
		catch (chn_receive(cp, &s));
		char c = chn_cast(s, '@');
		putchar(c);
	}
	putchar('\n');
	return thrd_success;
#	undef catch
}

int main(int argc, char* argv[])
{
	int err, status;
#	define catch(X)	if ((err=(X))!=thrd_success) goto onerror

	Channel chn;
	catch (chn_init(&chn, N));

	thrd_t p, c;
	catch (thrd_create(&p, producer, &chn));
	catch (thrd_create(&c, consumer, &chn));

	catch (thrd_join(p, &status)); catch (status);
	catch (thrd_join(c, &status)); catch (status);

	chn_destroy(&chn);

	return EXIT_SUCCESS;

onerror:
#	undef catch
	enum { internal_error };
	static const char* ename[] = {
		[thrd_success] = "thrd_success",
		[thrd_nomem] = "thrd_nomem",
		[thrd_busy] = "thrd_busy",
		[thrd_error] = "thrd_error",
		[thrd_timedout] = "thrd_timedout",
	};
	assert(err != thrd_success);
	switch (err) {
		case thrd_nomem:
			error(EXIT_FAILURE, ENOMEM, "%s", ename[err]);
		case thrd_busy:
		case thrd_error:
		case thrd_timedout:
			error(EXIT_FAILURE, 0, "%s", ename[err]);
		default: assert(internal_error);
	}
	return EXIT_FAILURE;
}

// vim:ai:sw=4:ts=4
