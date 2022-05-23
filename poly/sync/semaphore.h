#ifndef POLY_SEMAPHORE_H
#define POLY_SEMAPHORE_H
#define SEMAPHORE_IMPLEMENTED_USING_NOTICE 0

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../monitor/lock.h"
#if SEMAPHORE_IMPLEMENTED_USING_NOTICE
#include "../monitor/notice.h"
#else
#include "../monitor/condition.h"
#endif

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef struct Semaphore {
	Lock    syncronized;
#if SEMAPHORE_IMPLEMENTED_USING_NOTICE
	Notice  queue;
	// counter <  0: abs(#) of blocked threads;
	// counter == 0: idle;
	// counter >  0: value (available resources)
	signed  counter;
#else
	Condition queue;
	// counter <  0: error
	// counter == 0: idle;
	// counter >  0: value (available resources)
	signed  counter;
#endif
} Semaphore;

static void semaphore_destroy(Semaphore *const this);
static int  semaphore_init(Semaphore *const this, unsigned count);
static int  semaphore_P(Semaphore *const this);
static int  semaphore_V(Semaphore *const this);

#define     semaphore_acquire(s) semaphore_P(s)
#define     semaphore_release(s) semaphore_V(s)

#define     semaphore_wait(s) semaphore_P(s)
#define     semaphore_signal(s) semaphore_V(s)

#define     semaphore_down(s) semaphore_P(s)
#define     semaphore_up(s) semaphore_V(s)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

static int
semaphore_init (Semaphore *const this, unsigned count)
{
	this->counter = count;

	int err;
	if ((err=(lock_init(&this->syncronized))) != STATUS_SUCCESS) {
		return err;
	}
#if SEMAPHORE_IMPLEMENTED_USING_NOTICE
	if ((err=notice_init(&this->queue, &this->syncronized)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
#else
	if ((err=condition_init(&this->queue)) != STATUS_SUCCESS) {
		lock_destroy(&this->syncronized);
		return err;
	}
#endif
	return STATUS_SUCCESS;
}

static void
semaphore_destroy (Semaphore *const this)
{
	assert(this->counter == 0 ); // ???

#if SEMAPHORE_IMPLEMENTED_USING_NOTICE
	notice_destroy(&this->queue);
#else
	condition_destroy(&this->queue);
#endif
	lock_destroy(&this->syncronized);
}

/*
 * catch (semaphore_init(&mutex, 1));
 * catch (semaphore_acquire(&mutex))
 * ...
 * catch (semaphore_release(&mutex))
 *
 * catch (semaphore_init(&event, 0));
 * catch (semaphore_wait(&event)) | catch (semaphore_signal(&event))
 *
 * catch (semaphore_init(&allocator, N));
 */

static int
semaphore_P (Semaphore *const this)
{
	int err;
	enter_monitor(this);

#if SEMAPHORE_IMPLEMENTED_USING_NOTICE
	--this->counter;
	unsigned const waiting = (this->counter < 0) ? -this->counter : 0;
	if (waiting > 0) {
		catch (notice_do_wait(&this->queue));
	}
#else
	while (this->counter == 0) {
		catch (condition_wait(&this->queue, &this->syncronized));
	}
	--this->counter;
#endif

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

static int
semaphore_V (Semaphore *const this)
{
	int err;
	enter_monitor(this);

#if SEMAPHORE_IMPLEMENTED_USING_NOTICE
	unsigned const waiting = (this->counter < 0) ? -this->counter : 0;
	++this->counter;
	if (waiting > 0) {
		catch (notice_signal(&this->queue));
	}
#else
	++this->counter;
	// optimization: signal only when (this->counter == 1)?
	catch (condition_signal(&this->queue));
#endif

	leave_monitor(this);
	return STATUS_SUCCESS;
onerror:
	break_monitor(this);
	return err;
}

#undef SEMAPHORE_IMPLEMENTED_USING_NOTICE

#endif // vim:ai:sw=4:ts=4:syntax=cpp
