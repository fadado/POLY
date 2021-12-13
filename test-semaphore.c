// Semaphore test

#include <stdlib.h>
//#include <time.h>
#include <threads.h>

// uncomment next line to enable assertions
#define DEBUG
#include "poly.h"
#include "scalar.h"
#include "semaphore.h"

static Semaphore test_lock_mutex;
static Integer test_lock_counter;
enum { N=10, M=10000 };
int test_lock_task(void* args)
{
#ifdef DEBUG
	//warn("Enter %s", __func__);
#endif
	//struct timespec ts = {.tv_sec=0, .tv_nsec=1000, };
	//
	for (int i=0; i < M; ++i) {
		sem_acquire(&test_lock_mutex);
		Integer* pi = &test_lock_counter;
		Integer** ppi = &pi;
		Integer tmp = (**ppi-1) + 2;
		**ppi = tmp;
		sem_release(&test_lock_mutex);
	}
	thrd_yield();

	//thrd_sleep(&ts, NULL);
	//thrd_yield();
}
static void test_lock(void)
{
#ifdef DEBUG
	warn("Enter %s", __func__);
#endif
	test_lock_counter = 0;

	sem_init(&test_lock_mutex, 1);

	thrd_t t[N];
	for (int i=0; i < N; ++i) {
		int e = thrd_create(&t[i], test_lock_task, NULL);
		assert(e == thrd_success);
	}
	for (int i=0; i < N; ++i) {
		int e = thrd_join(t[i], NULL);
		assert(e == thrd_success);
	}
	assert(test_lock_counter == N*M);
	warn("COUNT: %d\n", test_lock_counter);
}

int main(int argc, char* argv[])
{
	test_lock();

	return EXIT_SUCCESS;
}

// vim:ai:sw=4:ts=4:syntax=cpp
