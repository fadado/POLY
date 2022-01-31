// Channel test
// gcc -Wall -O2 -I. -lpthread tests/filename.c

#include <stdio.h>
#include <stdlib.h>

// uncomment next line to enable assertions
#define DEBUG
#include "spinner.h"
#include "scalar.h"
#include "channel.h"

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
#	define catch(X)	if ((err=(X))!=STATUS_SUCCESS) return err

#ifdef DEBUG
	warn("Enter %s", __func__);
#endif
	Channel* channel = arg;
	for (int i=0; i < M; ++i) {
		char c = '0'+i;
		catch (channel_send(channel, (Unsigned)c));
#ifdef DEBUG
		//warn("Snd> %c", c);
#endif
	}
	channel_close(channel);
	// FAILURE: channel_send(channel, (Unsigned)'0');
#ifdef DEBUG
	warn("Exit %s", __func__);
#endif
	return STATUS_SUCCESS;
#	undef catch
}

/*
 *
 */
static int task_consumer(void* arg)
{
	int err;
#	define catch(X)	if ((err=(X))!=STATUS_SUCCESS) return err

#ifdef DEBUG
	warn("Enter %s", __func__);
#endif
	Channel* channel = arg;
	Scalar s;
	while (!channel_drained(channel)) {
		catch (channel_receive(channel, &s));
		char c = cast(s, char);
#ifdef DEBUG
		//warn("Rcv< %c", s);
#endif
		putchar(c);
	}
	putchar('\n');
#ifdef DEBUG
	warn("Exit %s", __func__);
#endif
	return STATUS_SUCCESS;
#	undef catch
}

/*
 *
 */
int main(int argc, char* argv[])
{
	int err, status;
#	define catch(X)	if ((err=(X))!=STATUS_SUCCESS) goto onerror

	Channel channel;
	catch (channel_init(&channel, N));

	Thread producer, consumer;

	catch (thread_fork(task_producer, &channel, &producer));
	catch (thread_fork(task_consumer, &channel, &consumer));

	catch (thread_join(producer, &status)); catch (status);
	catch (thread_join(consumer, &status)); catch (status);

	channel_destroy(&channel);

	return EXIT_SUCCESS;
onerror:
#	undef catch
	static const char* ename[] = {
		[STATUS_SUCCESS] = "thrd_success",
		[STATUS_NOMEM] = "thrd_nomem",
		[STATUS_BUSY] = "thrd_busy",
		[STATUS_ERROR] = "thrd_error",
		[STATUS_TIMEDOUT] = "thrd_timedout",
	};

	assert(err != STATUS_SUCCESS);
	switch (err) {
		case STATUS_NOMEM:
			error(EXIT_FAILURE, ENOMEM, "%s", ename[err]);
		case STATUS_BUSY:
			error(EXIT_FAILURE, EBUSY, "%s", ename[err]);
		case STATUS_ERROR:
			error(EXIT_FAILURE, ECANCELED, "%s", ename[err]);
		case STATUS_TIMEDOUT:
			error(EXIT_FAILURE, ETIMEDOUT, "%s", ename[err]);
		default: assert(internal_error);
	}
	return EXIT_FAILURE;
}

// vim:ai:sw=4:ts=4:syntax=cpp
