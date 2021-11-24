/*
 * Channels of scalars
 *
 * Compile: gcc -O2 -lpthread ...
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stdlib.h>
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
	unsigned back;
	unsigned size;
	unsigned count;
	mtx_t    monitor;
	cnd_t    nonempty;
	cnd_t    nonfull;
	Scalar*  buffer;
} Channel;

#define ALWAYS __attribute__((always_inline))

static        inline int  chn_init(Channel* self, unsigned capacity);
static        inline void chn_destroy(Channel* self);
static ALWAYS inline int  chn_send_(Channel* self, Scalar x);
static ALWAYS inline int  chn_receive(Channel* self, Scalar* x);

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

//
// Channel life
//
static inline int chn_init(Channel* self, unsigned capacity)
{
	self->count = self->front = self->back = 0;
	self->size  = (capacity == 0 ? 1 : capacity);

	self->buffer = calloc(self->size, sizeof(Scalar));
	if (self->buffer == (Scalar*)0) return thrd_nomem;

	int err, eN=0;
#	define catch(X)	if ((++eN,err=(X))!=thrd_success) goto onerror

	catch (mtx_init(&self->monitor, mtx_plain)); // eN == 1
	catch (cnd_init(&self->nonempty));           // eN == 2
	catch (cnd_init(&self->nonfull));            // eN == 3

	return thrd_success;

onerror:
#	undef catch
	assert(err != thrd_success);
	assert(1 <= eN && eN <= 3);
	switch (eN) {
		case 3: cnd_destroy(&self->nonempty);
		case 2: mtx_destroy(&self->monitor);
		case 1: free(self->buffer);
		default: return err;
	}
}

static inline void chn_destroy(Channel* self)
{
	assert(self->count == 0);

	free(self->buffer);
	mtx_destroy(&self->monitor);
	cnd_destroy(&self->nonempty);
	cnd_destroy(&self->nonfull);

	// sanitize
	self->count = self->front = self->back = self->size = 0;
	self->buffer = (Scalar*)0;
}

//
// Send & Receive
//
static ALWAYS inline int chn_send_(Channel* self, Scalar x)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success)

	catch (mtx_lock(&self->monitor)) { return err; }

	if (_chn_full(self)) {
		catch (cnd_wait(&self->nonfull, &self->monitor)) {
			mtx_destroy(&self->monitor);
			return err;
		}
	}

	self->buffer[self->front] = x;
	self->front = (self->front+1) % self->size;
	++self->count;

	catch (mtx_unlock(&self->monitor)) {
		cnd_signal(&self->nonempty);
		return err;
	}
	catch (cnd_signal(&self->nonempty)) { return err; }

	return thrd_success;
#	undef catch
}

static ALWAYS inline int chn_receive(Channel* self, Scalar* x)
{
	int err;
#	define catch(X)	if ((err=(X))!=thrd_success)

	catch (mtx_lock(&self->monitor)) { return err; }

	if (_chn_empty(self)) {
		catch (cnd_wait(&self->nonempty, &self->monitor)) {
			mtx_destroy(&self->monitor);
			return err;
		}
	}

	*x = self->buffer[self->back];
	self->back = (self->back+1) % self->size;
	--self->count;

	catch (mtx_unlock(&self->monitor)) {
		cnd_signal(&self->nonfull);
		return err;
	}
	catch (cnd_signal(&self->nonfull)) { return err; }

	return thrd_success;
#	undef catch
}

#undef ALWAYS

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4
