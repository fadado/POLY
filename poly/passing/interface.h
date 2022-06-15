#ifndef POLY_INTERFACE_H
#define POLY_INTERFACE_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "entry.h"

////////////////////////////////////////////////////////////////////////
// Rendezvous
////////////////////////////////////////////////////////////////////////

static int  interface_init(unsigned n, Entry entries[]);
static void interface_destroy(unsigned n, Entry entries[]);

static int
interface_init (unsigned n, Entry entries[])
{
	assert(n > 0);
	for (unsigned i = 0; i < n; ++i) {
		const int err = entry_init(&entries[i]);
		if (err != STATUS_SUCCESS) {
			if (i > 0) {
				interface_destroy(i, entries);
			}
			return err;
		}
	}
	return STATUS_SUCCESS;
}

#define interface_init(N,E) interface_init((N), (Entry*)(E))

static void
interface_destroy (unsigned n, Entry entries[])
{
	assert(n > 0);
	do {
		--n;
		entry_destroy(&entries[n]);
	} while (n != 0);
}

#define interface_destroy(N,E) interface_init((N), (Entry*)(E))

////////////////////////////////////////////////////////////////////////
// Interface definition
////////////////////////////////////////////////////////////////////////

#define INTERFACE_TYPE(T)   struct T##_face {

#define INTERFACE_SLOT(T)   struct T##_face * IFACE_

#define interface(T)        struct T##_face

#define run_task(T,I,...) \
    run_thread(T, .IFACE_=(I) __VA_OPT__(,)__VA_ARGS__)

#define ENTRIES(I)  (sizeof(I)/sizeof(Entry))

/*
 *
 *  INTERFACE_TYPE (T)
 *      Entry e1;
 *      Entry e2;
 *      ...
 *  END_TYPE
 *
 *  THREAD_TYPE (T)
 *      ...
 *      INTERFACE_SLOT (T);
 *  END_TYPE
 *
 *  static interface(Printer) IPrinter;
 *  interface_init(n, &IPrinter);
 *  err = run_task(Printer,...);
 *  Scalar r;
 *  entry_call(&IPrinter.e1, s, &r);
 */

////////////////////////////////////////////////////////////////////////
// Select statement
////////////////////////////////////////////////////////////////////////

#define select          int _open=0; int _selec=0;

#define entry(E)        this.IFACE_->E

#define when(G,E)       if ((G) && ++_open && entry_ready(entry(E)) && ++_selec)

#define terminate       if (!_open) panic("ops!") elif (!_selec) break else continue
 
/*
 *
 *  for (;;) {
 *      select {
 *          when (guard, e1) {
 *              void thunk(void) {
 *                  Scalar s = f(entry(e1)->request);
 *                  entry(e1)->response = s;
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

#undef ASSERT_INTERFACE_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
