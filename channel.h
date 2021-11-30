/*
 * Channels of scalars
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stdlib.h> // calloc
#include <threads.h>

#ifndef FAILURE_H
#error To cope with failure I need "failure.h"!
#endif

////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////

typedef signed long long   Integer;
typedef unsigned long long Word;
typedef double             Real;
typedef void*              Pointer;

typedef union {
	Integer integer;
	Word    word;
	Real    real;
	Pointer pointer;
} Scalar
__attribute__((__transparent_union__));

static_assert(sizeof(Real)==sizeof(Integer));
static_assert(sizeof(Real)==sizeof(Word));
static_assert(sizeof(Real)==sizeof(Pointer));
static_assert(sizeof(Scalar)==8);

typedef struct {
	short unsigned flags;
	short count;
	short front;
	short size;
	union {
		Scalar* buffer;
		Scalar  value;
	};
	mtx_t lock;
	cnd_t non_empty;
	cnd_t non_full;
} Channel;

enum channel_flag {
	CHANNEL_BUFFERED    = (1<<0),
	CHANNEL_ASYNCRONOUS = (1<<0), // synonym for buffered
	CHANNEL_BLOCKING    = (1<<1),
	CHANNEL_UNBUFFERED  = (1<<1), // synonym for blocking
	CHANNEL_CLOSED      = (1<<2),
};

#ifdef DEBUG
#	define ASSERT_CHANNEL_INVARIANT\
		assert(0 <= self->count && self->count <= self->size);\
		assert(0 <= self->front && self->front <  self->size);\
		assert(self->size < 2 || self->buffer != (Scalar*)0);\
		assert(!(_chn_empty(self) && _chn_full(self)));
#else
#	define ASSERT_CHANNEL_INVARIANT
#endif

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

static inline int  chn_init(Channel* self, unsigned capacity);
static inline void chn_destroy(Channel* self);
static inline int  chn_send_(Channel* self, Scalar x);
static inline int  chn_receive(Channel* self, Scalar* x);
static inline void chn_close(Channel* self);
static inline bool chn_exhaust(Channel* self);
//                 chn_cast(SCALAR,EXPRESSION) ...

// Accept any scalar type
#define chn_send(CHANNEL,EXPRESSION) chn_send_((CHANNEL), _Generic((EXPRESSION),\
	Scalar: (EXPRESSION),\
	_Bool: (Word)(EXPRESSION),\
	char: (Word)(EXPRESSION),\
	signed char: (Integer)(EXPRESSION),\
	unsigned char: (Word)(EXPRESSION),\
	signed short int: (Integer)(EXPRESSION),\
	unsigned short int: (Word)(EXPRESSION),\
	signed int: (Integer)(EXPRESSION),\
	unsigned int: (Word)(EXPRESSION),\
	signed long int: (Integer)(EXPRESSION),\
	unsigned long int: (Word)(EXPRESSION),\
	signed long long int: (Integer)(EXPRESSION),\
	unsigned long long int: (Word)(EXPRESSION),\
	float: (Real)(EXPRESSION),\
	double: (Real)(EXPRESSION),\
	long double: (Real)(EXPRESSION),\
	default: (Pointer)(Word)(EXPRESSION)))

// Cast an Scalar to the expression type
#define chn_cast(SCALAR,EXPRESSION) _Generic((EXPRESSION),\
	_Bool: (_Bool)(SCALAR).word,\
	char: (char)(SCALAR).word,\
	signed char: (signed char)(SCALAR).integer,\
	unsigned char: (unsigned char)(SCALAR).word,\
	signed short int: (signed short int)(SCALAR).integer,\
	unsigned short int: (unsigned short int)(SCALAR).word,\
	signed int: (signed int)(SCALAR).integer,\
	unsigned int: (unsigned int)(SCALAR).word,\
	signed long int: (signed long int)(SCALAR).integer,\
	unsigned long int: (unsigned long int)(SCALAR).word,\
	signed long long int: (signed long long int)(SCALAR).integer,\
	unsigned long long int: (unsigned long long int)(SCALAR).word,\
	float: (float)(SCALAR).real,\
	double: (double)(SCALAR).real,\
	long double: (long double)(SCALAR).real,\
	default: (void*)(SCALAR).pointer)

////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////

#define ALWAYS __attribute__((always_inline))

//
// Predicates
//
static ALWAYS inline bool _chn_empty(Channel* self)
{
	return self->count == 0;
}

static ALWAYS inline bool _chn_full(Channel* self)
{
	return self->count == self->size;
}

static ALWAYS inline bool _chn_flag(Channel* self, enum channel_flag flag)
{
	return (self->flags & flag) == flag;
}

/*
 *
 */
static ALWAYS inline bool chn_exhaust(Channel* self)
{
	mtx_lock(&self->lock);
	bool exhaust = _chn_flag(self, CHANNEL_CLOSED) && _chn_empty(self);
	mtx_unlock(&self->lock);
	return exhaust;
}

//
// Channel life
//

/*
 *
 */
static inline int chn_init(Channel* self, unsigned capacity)
{
	if (capacity > 0x7FFFu) return thrd_error;

	self->count = self->front = self->flags = 0;
	self->size = capacity;
	switch (self->size) {
		case 0:
			// TODO:
			assert(not_implemented);
			self->flags |= CHANNEL_BLOCKING;
			break;
		case 1:
			self->flags |= CHANNEL_BUFFERED;
			break;
		default:
			self->flags |= CHANNEL_BUFFERED;
			self->buffer = calloc(self->size, sizeof(Scalar));
			if (self->buffer == (Scalar*)0) return thrd_nomem;
			break;
	}
	ASSERT_CHANNEL_INVARIANT

	// initialize mutex and conditions
	int err, eN=0;
#	define catch(X)	if ((++eN,err=(X))!=thrd_success) goto onerror

	catch (mtx_init(&self->lock, mtx_plain)); // eN == 1
	catch (cnd_init(&self->non_empty));       // eN == 2
	catch (cnd_init(&self->non_full));        // eN == 3

	return thrd_success;
onerror:
#	undef catch
	assert(err != thrd_success);

	assert(1 <= eN && eN <= 3);
	switch (eN) {
		case 3: cnd_destroy(&self->non_empty);
		case 2: mtx_destroy(&self->lock);
		case 1: if (self->size > 1) free(self->buffer);
	}
	return err;
}

/*
 *
 */
static inline void chn_destroy(Channel* self)
{
	assert(self->count == 0); // ???

	switch (self->size) {
		case 0: // blocking channel
			// TODO:
			assert(not_implemented);
			break;
		case 1: // buffered channel
			break;
		default:
			free(self->buffer);
			self->buffer = (Scalar*)0; // sanitize
			break;
	}
	mtx_destroy(&self->lock);
	cnd_destroy(&self->non_empty);
	cnd_destroy(&self->non_full);
}

/*
 *
 */
static ALWAYS inline void chn_close(Channel* self)
{
	mtx_lock(&self->lock);
	self->flags |= CHANNEL_CLOSED;
	mtx_unlock(&self->lock);
}

//
// Monitor helpers
//

#define ENTER_CHANNEL_MONITOR(PREDICATE,CONDITION)\
	int err_;\
	if ((err_=mtx_lock(&self->lock))!=thrd_success) {\
		return err_;\
	}\
	while (PREDICATE(self)) {\
		if ((err_=cnd_wait(&CONDITION, &self->lock))!=thrd_success) {\
			mtx_destroy(&self->lock);\
			return err_;\
		}\
	}

#ifdef VersionWithSignalAfterUnlock
#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if ((err_=mtx_unlock(&self->lock))!=thrd_success) {\
		cnd_signal(&CONDITION);\
		return err_;\
	}\
	if ((err_=cnd_signal(&CONDITION))!=thrd_success) {\
		return err_;\
	}
#endif

#define LEAVE_CHANNEL_MONITOR(CONDITION)\
	if ((err_=cnd_signal(&CONDITION))!=thrd_success) {\
		mtx_unlock(&self->lock);\
		return err_;\
	}\
	if ((err_=mtx_unlock(&self->lock))!=thrd_success) {\
		return err_;\
	}

//
// FIFO
//

/*
 *
 */
static inline int chn_send_(Channel* self, Scalar x)
{
	ENTER_CHANNEL_MONITOR (_chn_full, self->non_full)

	if (_chn_flag(self, CHANNEL_CLOSED)) {
		panic("chn_send want to send an scalar to a closed channel");
	}

	switch (self->size) {
		case 0: // blocking channel
			// TODO:
			assert(not_implemented);
		case 1: // buffered channel
			assert(self->count == 0);
			self->value = x;
			break;
		default:
			self->buffer[self->front] = x;
			self->front = (self->front+1) % self->size;
			break;
	}
	++self->count;
	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (self->non_empty)

	return thrd_success;
}

/*
 *
 */
static inline int chn_receive(Channel* self, Scalar* x)
{
	ENTER_CHANNEL_MONITOR (_chn_empty, self->non_empty)

	switch (self->size) {
		case 0: // blocking channel
			// TODO:
			assert(not_implemented);
		case 1: // buffered channel
			assert(self->count == 1);
			if (x) *x = self->value;
			break;
		default:
			if (x) {
				int back = self->front - self->count;
				back = (back >= 0) ? back : back+self->size;
				*x = self->buffer[back];
			}
			break;
	}
	--self->count;
	ASSERT_CHANNEL_INVARIANT

	LEAVE_CHANNEL_MONITOR (self->non_full)

	return thrd_success;
}

#undef ALWAYS
#undef ASSERT_CHANNEL_INVARIANT
#undef ENTER_CHANNEL_MONITOR
#undef LEAVE_CHANNEL_MONITOR

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4:syntax=cpp
