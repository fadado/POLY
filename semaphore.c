// Semaphore test
// gcc -Wall -O2 -lpthread filename.c

#include <stdlib.h>
//#include <time.h>

// uncomment next line to enable assertions
#define DEBUG
#include "poly/scalar.h"
#include "poly/thread.h"
#include "poly/sync/semaphore.h"

static Semaphore test_lock_mutex;
static Signed test_lock_counter;
enum { N=10, M=10000 };

int thread_test(void* arg)
{
	assert(arg == NULL);
#ifdef DEBUG
	//warn("Enter %s", __func__);
#endif
	for (int i=0; i < M; ++i) {
		semaphore_acquire(&test_lock_mutex);
		Signed* pi = &test_lock_counter;
		Signed** ppi = &pi;
		Signed tmp = (**ppi-1) + 2;
		**ppi = tmp;
		semaphore_release(&test_lock_mutex);
	}
	thread_yield();

	return 0;
}

static void test_lock(void)
{
#ifdef DEBUG
	warn("Enter %s", __func__);
#endif
	test_lock_counter = 0;

	semaphore_init(&test_lock_mutex, 1);

	Thread t[N];
	for (int i=0; i < N; ++i) {
		int e = thread_fork(thread_test, NULL, &t[i]);
		assert(e == STATUS_SUCCESS);
	}
	for (int i=0; i < N; ++i) {
		int e = thread_join(t[i], NULL);
		assert(e == STATUS_SUCCESS);
	}
	assert(test_lock_counter == N*M);
	warn("COUNT: %lld\n", test_lock_counter);

	//semaphore_destroy(&test_lock_mutex);
}

int main(int argc, char* argv[argc+1])
{
	test_lock();

	return EXIT_SUCCESS;
}

// vim:ai:sw=4:ts=4:syntax=cpp
