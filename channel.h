/*
 * Channels of scalars
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h> // calloc
#include <threads.h>

// comment next line to enable assertions
//#define NDEBUG
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef signed long long   Signed;
typedef unsigned long long Unsigned;
typedef double             Real;
typedef void*              Pointer;

typedef union {
	Signed   i;
	Unsigned u;
	Real     r;
	Pointer  p;
} Scalar
__attribute__((__transparent_union__));

static_assert(sizeof(Real)==sizeof(Signed));
static_assert(sizeof(Real)==sizeof(Unsigned));
static_assert(sizeof(Real)==sizeof(Pointer));
static_assert(sizeof(Scalar)==8);

typedef struct {
	unsigned front;
	unsigned size;
	unsigned count;
	mtx_t    lock;
	cnd_t    nonempty;
	cnd_t    nonfull;
	union {
		Scalar* buffer;
		Scalar  value;
	};
} Channel;

static inline int  chn_init(Channel* self, unsigned capacity);
static inline void chn_destroy(Channel* self);
static inline int  chn_send_(Channel* self, Scalar x);
static inline int  chn_receive(Channel* self, Scalar* x);

// Accept any scalar type
#define chn_send(CHANNEL,EXPRESSION) chn_send_((CHANNEL), _Generic((EXPRESSION),\
	Scalar: (EXPRESSION),\
	_Bool: (Unsigned)(EXPRESSION),\
	char: (Unsigned)(EXPRESSION),\
	signed char: (Signed)(EXPRESSION),\
	unsigned char: (Unsigned)(EXPRESSION),\
	signed short int: (Signed)(EXPRESSION),\
	unsigned short int: (Unsigned)(EXPRESSION),\
	signed int: (Signed)(EXPRESSION),\
	unsigned int: (Unsigned)(EXPRESSION),\
	signed long int: (Signed)(EXPRESSION),\
	unsigned long int: (Unsigned)(EXPRESSION),\
	signed long long int: (Signed)(EXPRESSION),\
	unsigned long long int: (Unsigned)(EXPRESSION),\
	float: (Real)(EXPRESSION),\
	double: (Real)(EXPRESSION),\
	long double: (Real)(EXPRESSION),\
	default: (Pointer)(Unsigned)(EXPRESSION)))

// Cast an Scalar to the expression type
#define chn_cast(SCALAR,EXPRESSION) _Generic((EXPRESSION),\
	_Bool: (_Bool)(SCALAR).u,\
	char: (char)(SCALAR).u,\
	signed char: (signed char)(SCALAR).i,\
	unsigned char: (unsigned char)(SCALAR).u,\
	signed short int: (signed short int)(SCALAR).i,\
	unsigned short int: (unsigned short int)(SCALAR).u,\
	signed int: (signed int)(SCALAR).i,\
	unsigned int: (unsigned int)(SCALAR).u,\
	signed long int: (signed long int)(SCALAR).i,\
	unsigned long int: (unsigned long int)(SCALAR).u,\
	signed long long int: (signed long long int)(SCALAR).i,\
	unsigned long long int: (unsigned long long int)(SCALAR).u,\
	float: (float)(SCALAR).r,\
	double: (double)(SCALAR).r,\
	long double: (long double)(SCALAR).r,\
	default: (void*)(SCALAR).p)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#define ALWAYS __attribute__((always_inline))

#ifdef NDEBUG
#	define ASSERT_CHANNEL_INVARIANT
#else
#	define ASSERT_CHANNEL_INVARIANT\
		assert(0 <= self->count && self->count <= self->size);\
		assert(0 <= self->front && self->front <  self->size);
#endif

//
// Predicates
//
static ALWAYS inline unsigned _chn_empty(Channel* self)
{
	return self->count == 0;
}

static ALWAYS inline unsigned _chn_full(Channel* self)
{
	return self->count == self->size;
}

//
// Channel life
//
static inline int chn_init(Channel* self, unsigned capacity)
{
	self->count = self->front = 0;
	if (capacity > 1) {
		self->size = capacity;
		self->buffer = calloc(self->size, sizeof(Scalar));
		if (self->buffer == (Scalar*)0) return thrd_nomem;
	} else {
		self->size = 1;
		assert(0);
		// TODO: use self->value
	}

	int err, eN=0;
#	define catch(X)	if ((++eN,err=(X))!=thrd_success) goto onerror

	catch (mtx_init(&self->lock, mtx_plain)); // eN == 1
	catch (cnd_init(&self->nonempty));        // eN == 2
	catch (cnd_init(&self->nonfull));         // eN == 3

	ASSERT_CHANNEL_INVARIANT

	return thrd_success;

onerror:
#	undef catch
	assert(err != thrd_success);
	assert(1 <= eN && eN <= 3);
	switch (eN) {
		case 3: cnd_destroy(&self->nonempty);
		case 2: mtx_destroy(&self->lock);
		case 1: if (self->size > 1) free(self->buffer);
	}
	return err;
}

static inline void chn_destroy(Channel* self)
{
	assert(self->count == 0); // ???

	if (self->size > 1){
		free(self->buffer);
		self->buffer = (Scalar*)0; // sanitize
	}
	mtx_destroy(&self->lock);
	cnd_destroy(&self->nonempty);
	cnd_destroy(&self->nonfull);
}

////////////////////////////////////////////////////////////////////////

#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	int err_;\
	if ((err_=mtx_lock(&self->lock))!=thrd_success)\
		{ return err_; }\
	/*if?*/while (PREDICATE) {\
		if ((err_=cnd_wait(CONDITION, &self->lock))!=thrd_success) {\
			mtx_destroy(&self->lock);\
			return err_;\
		}\
	}

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if ((err_=mtx_unlock(&self->lock))!=thrd_success) {\
		cnd_signal(CONDITION);\
		return err_;\
	}\
	if ((err_=cnd_signal(CONDITION))!=thrd_success)\
		{ return err_; }

////////////////////////////////////////////////////////////////////////

//
// Send & Receive
//
static inline int chn_send_(Channel* self, Scalar x)
{
	ENTER_CHANNEL_MONITOR (_chn_full(self), &self->nonfull)

	self->buffer[self->front] = x;
	self->front = (self->front+1) % self->size;
	++self->count;

	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (&self->nonempty)

	return thrd_success;
}

static inline int chn_receive(Channel* self, Scalar* x)
{
	inline int back() {
		int b = self->front - self->count;
		return (b >= 0) ? b : b+self->size;
	}

	ENTER_CHANNEL_MONITOR (_chn_empty(self), &self->nonempty)

	if (x) *x = self->buffer[back()];
	--self->count;

	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (&self->nonfull)

	return thrd_success;
}

#undef ALWAYS
#undef ASSERT_CHANNEL_INVARIANT
#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
