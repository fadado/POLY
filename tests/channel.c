// Channel test
// gcc -Wall -O2 -I. -lpthread tests/filename.c

#include <stdio.h>
#include <stdlib.h>

// uncomment next line to enable assertions
#define DEBUG
#include "channel.h" // include scalar.h

////////////////////////////////////////////////////////////////////////
// FIFO test
////////////////////////////////////////////////////////////////////////

#define N 0
#define M 7

/*
 *
 */
static int task_producer(void* arg)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) return err

#ifdef DEBUG
	warn("Enter %s", __func__);
#endif
	Channel* channel = arg;
	for (int i=0; i < M; ++i) {
		char c = '0'+i;
		catch (channel_send(channel, c));
#ifdef DEBUG
		//warn("Snd> %c", c);
#endif
	}
	channel_close(channel);
	// FAILURE: channel_send(channel, '0');
#ifdef DEBUG
	warn("Exit %s", __func__);
#endif
	return thrd_success;
#	undef catch
}

/*
 *
 */
static int task_consumer(void* arg)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success) return err

#ifdef DEBUG
	warn("Enter %s", __func__);
#endif
	Channel* channel = arg;
	Scalar s;
	while (!channel_drained(channel)) {
		catch (channel_receive(channel, &s));
		char c = cast(s, '@');
#ifdef DEBUG
		//warn("Rcv< %c", s);
#endif
		putchar(c);
	}
	putchar('\n');
#ifdef DEBUG
	warn("Exit %s", __func__);
#endif
	return thrd_success;
#	undef catch
}

/*
 *
 */
int main(int argc, char* argv[])
{
	int err, status;
#	define catch(X)	if ((err=(X))!=thrd_success) goto onerror

	Channel channel;
	catch (channel_init(&channel, N));

	thrd_t producer, consumer;

	catch (thrd_create(&producer, task_producer, &channel));
	catch (thrd_create(&consumer, task_consumer, &channel));

	catch (thrd_join(producer, &status)); catch (status);
	catch (thrd_join(consumer, &status)); catch (status);

	channel_destroy(&channel);

	return EXIT_SUCCESS;
onerror:
#	undef catch
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
			error(EXIT_FAILURE, EBUSY, "%s", ename[err]);
		case thrd_error:
			error(EXIT_FAILURE, ECANCELED, "%s", ename[err]);
		case thrd_timedout:
			error(EXIT_FAILURE, ETIMEDOUT, "%s", ename[err]);
		default: assert(internal_error);
	}
	return EXIT_FAILURE;
}

// vim:ai:sw=4:ts=4:syntax=cpp
