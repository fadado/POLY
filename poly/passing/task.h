#ifndef POLY_TASK_H
#define POLY_TASK_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "entry.h"

////////////////////////////////////////////////////////////////////////
// Rendezvous
////////////////////////////////////////////////////////////////////////

static int  task_init(unsigned n, Entry entries[]);
static void task_destroy(unsigned n, Entry entries[]);

#define ENTRIES(E)  (sizeof(E) / sizeof(Entry))

static int
task_init (unsigned n, Entry entries[])
{
	assert(n > 0);
	for (unsigned i = 0; i < n; ++i) {
		const int err = entry_init(&entries[i]);
		if (err != STATUS_SUCCESS) {
			if (i > 0) {
				task_destroy(i, entries);
			}
			return err;
		}
	}
	return STATUS_SUCCESS;
}

#define task_init(N,E) task_init((N), (Entry*)(E))

static void
task_destroy (unsigned n, Entry entries[])
{
	assert(n > 0);
	do {
		--n;
		entry_destroy(&entries[n]);
	} while (n != 0);
}

#define task_destroy(N,E) task_destroy((N), (Entry*)(E))

////////////////////////////////////////////////////////////////////////
// Task specification
////////////////////////////////////////////////////////////////////////

#define TASK_TYPE(E)    \
	THREAD_TYPE         \
	E* entries_;

/*
 *
 *  typedef struct {
 *      Entry A;
 *      Entry B;
 *      ...
 *  } ET;
 *
 *  struct T {
 *      THREAD_TYPE
 *      TASK_TYPE (ET)
 *      ...
 *  };
 *
 *  int T(void* data)
 *  {
 *      THREAD_BODY (T, data)
 *      ...
 *      END_BODY
 *  }
 */

#define run_task(T,E,...) \
    run_thread(T, .entries_=(E) __VA_OPT__(,)__VA_ARGS__)

/*
 *  ET e;
 *  catch (task_init(ENTRIES(ET), &e));
 *  ...
	run_task(T, &e, ...);
 *  ...
 *  catch (task_destroy(ENTRIES(ET), &e));
 */

////////////////////////////////////////////////////////////////////////
// Select statement
////////////////////////////////////////////////////////////////////////

#define select          int open_=0; int selec_=0;

#define entry(E)        this.entries_->E

#define when(G,E)       if ((G) && ++open_ && entry_ready(entry(E)) && ++selec_)

#define terminate       if (!open_) panic("ops!") elif (!selec_) break else continue
 
/*
 *
 *  for (;;) {
 *      select {
 *          when (guard, e1) {
 *              void thunk(void) {
 *                  Scalar s = f(entry(e1)->query);
 *                  entry(e1)->reply = s;
 *              }
 *              catch (entry_accept(entry(e1), thunk));
 *          }
 *    //or
 *          when ...
 *    //or
 *          when ...
 *    //or
 *          terminate;
 *      }
 *  }
 */

#endif // vim:ai:sw=4:ts=4:syntax=cpp
