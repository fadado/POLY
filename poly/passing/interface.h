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

#define interface_destroy(N,E) interface_destroy((N), (Entry*)(E))

////////////////////////////////////////////////////////////////////////
// Interface definition
////////////////////////////////////////////////////////////////////////

#define INTERFACE_TYPE(T)   struct T##_face_ {

#define ENTRIES(I)          (sizeof(I)/sizeof(Entry))

#define interface(T)        struct T##_face_

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
 *  err = run_task(Printer &IPrinter,...);
 *  Scalar r;
 *  entry_call(&IPrinter.e1, s, &r);
 */

////////////////////////////////////////////////////////////////////////
// Select statement
////////////////////////////////////////////////////////////////////////

#define select          int open_=0; int selec_=0;

#define entry(E)        this.interface_->E

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
