// Semaphore test

#include <stdlib.h>
//#include <time.h>

// uncomment next line to enable assertions
#define DEBUG
#include "POLY.h"
#include "task.h"
#include "scalar.h"
#include "semaphore.h"

static Semaphore test_lock_mutex;
static Integer test_lock_counter;
enum { N=10, M=10000 };
int task_test(void* args)
{
#ifdef DEBUG
	//warn("Enter %s", __func__);
#endif
	for (int i=0; i < M; ++i) {
		sem_acquire(&test_lock_mutex);
		Integer* pi = &test_lock_counter;
		Integer** ppi = &pi;
		Integer tmp = (**ppi-1) + 2;
		**ppi = tmp;
		sem_release(&test_lock_mutex);
	}
	tsk_yield();

	return 0;
}
static void test_lock(void)
{
#ifdef DEBUG
	warn("Enter %s", __func__);
#endif
	test_lock_counter = 0;

	sem_init(&test_lock_mutex, 1);

	Task t[N];
	for (int i=0; i < N; ++i) {
		int e = tsk_fork(task_test, (void*)0, &t[i]);
		assert(e == STATUS_SUCCESS);
	}
	for (int i=0; i < N; ++i) {
		int e = tsk_join(t[i]);
		assert(e == STATUS_SUCCESS);
	}
	assert(test_lock_counter == N*M);
	warn("COUNT: %lld\n", test_lock_counter);
}

int main(int argc, char* argv[])
{
	test_lock();

	return EXIT_SUCCESS;
}

// vim:ai:sw=4:ts=4:syntax=cpp
