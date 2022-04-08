#ifndef FIFO_H
#define FIFO_H

#ifndef POLY_H
#include "../POLY.h"
#endif
#include "../scalar.h"

//#include <stdlib.h>
extern void  free(void*);
extern void* calloc(size_t, size_t);

////////////////////////////////////////////////////////////////////////
// Type FIFO buffer of Scalars
////////////////////////////////////////////////////////////////////////

typedef struct FIFO {
	Scalar*  buffer;
	unsigned size;
	unsigned count;
	unsigned front;
} FIFO;

static int    fifo_init(FIFO *const this, unsigned size);
static void   fifo_destroy(FIFO *const this);
static bool   fifo_empty(FIFO const*const this);
static bool   fifo_full(FIFO const*const this);
static void   fifo_put(FIFO *const this, Scalar scalar);
static Scalar fifo_get(FIFO *const this);

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#	define ASSERT_FIFO_INVARIANT\
		assert(0 <= this->count && this->count <= this->size);\
		assert(0 <= this->front && this->front < this->size);\
		assert(this->buffer != NULL);\
		assert(!(fifo_empty(this) && fifo_full(this)));
#else
#	define ASSERT_FIFO_INVARIANT
#endif

static int
fifo_init (FIFO *const this, unsigned size)
{
	this->size = size;
	this->front = this->count = 0;
	this->buffer = calloc(this->size, sizeof(Scalar));
	if (this->buffer == NULL) {
		return STATUS_NOMEM;
	}
	ASSERT_FIFO_INVARIANT
	return STATUS_SUCCESS;
}

static void
fifo_destroy (FIFO *const this)
{
	free(this->buffer);
	this->buffer = NULL;
}

static ALWAYS inline bool
fifo_empty (FIFO const*const this)
{
	return this->count == 0;
}

static ALWAYS inline bool
fifo_full (FIFO const*const this)
{
	return this->count == this->size;
}

static ALWAYS inline void
fifo_put (FIFO *const this, Scalar scalar)
{
	this->buffer[this->front] = scalar;
	this->front = (this->front+1) % this->size;
	++this->count;
	ASSERT_FIFO_INVARIANT
}

static ALWAYS inline Scalar
fifo_get (FIFO *const this)
{
	register int back = this->front - this->count;
	if (back < 0) {
		back += this->size;
	}
	--this->count;
	ASSERT_FIFO_INVARIANT
	return this->buffer[back];
}

#undef ASSERT_FIFO_INVARIANT

#endif // vim:ai:sw=4:ts=4:syntax=cpp
