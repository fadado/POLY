/*
 * Channels of scalars
 *
 *
 *
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>

// comment next line to enable assertions
//#define NDEBUG
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////

typedef union {
	int64_t  i;
	uint64_t u;
	double   r;
	void*    p;
} Scalar __attribute__((__transparent_union__));

typedef struct {
	unsigned front;
	unsigned back;
	unsigned size;
	Scalar*  buffer;
} Channel;

#define ALWAYS __attribute__((always_inline))

static        inline int  chn_init(Channel* self, unsigned capacity);
static        inline void chn_destroy(Channel* self);
static ALWAYS inline bool _chn_empty(Channel* self);
static ALWAYS inline bool _chn_full(Channel* self);
static ALWAYS inline int  chn_send_(Channel* self, Scalar x);
static ALWAYS inline int  chn_receive(Channel* self, Scalar* x);

// Accept any scalar type
#define chn_send(CHANNEL,SCALAR) chn_send_((CHANNEL), _Generic((SCALAR),\
	_Bool: (uint64_t)(SCALAR),\
	char: (uint64_t)(SCALAR),\
	signed char: (int64_t)(SCALAR),\
	unsigned char: (uint64_t)(SCALAR),\
	signed short int: (int64_t)(SCALAR),\
	unsigned short int: (uint64_t)(SCALAR),\
	signed int: (int64_t)(SCALAR),\
	unsigned int: (uint64_t)(SCALAR),\
	signed long int: (int64_t)(SCALAR),\
	unsigned long int: (uint64_t)(SCALAR),\
	signed long long int: (int64_t)(SCALAR),\
	unsigned long long int: (uint64_t)(SCALAR),\
	float: (double)(SCALAR),\
	double: (double)(SCALAR),\
	long double: (double)(SCALAR),\
	default: (void*)(intptr_t)(SCALAR)))/*assume pointer*/\

// Cast an Scalar to the variable type
#define chn_cast(SCALAR,VARIABLE) _Generic((VARIABLE),\
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
// Channel life
//
static inline int chn_init(Channel* self, unsigned capacity)
{
	static_assert(sizeof(double)==sizeof(int64_t));
	static_assert(sizeof(double)==sizeof(uint64_t));
	static_assert(sizeof(double)==sizeof(void*));
	static_assert(sizeof(Scalar)==8);

	self->front = self->back = 0;
	self->size  = 1+(capacity==0 ? 1 : capacity);

	self->buffer = calloc(self->size, sizeof(Scalar));
	if (!self->buffer)
		return thrd_nomem;

	//assert(_chn_empty(self));
	//assert(!_chn_full(self));

	return thrd_success;
}

static inline void chn_destroy(Channel* self)
{
	free(self->buffer);

	// sanitize
	self->front = self->back = self->size = 0;
	self->buffer = (Scalar*)0;
}

//
// Predicates
//
static ALWAYS inline bool _chn_empty(Channel* self)
{
	return self->front == self->back;
}

static ALWAYS inline bool _chn_full(Channel* self)
{
	return ((self->front+1) % self->size) == self->back;
}

//
// Send & Receive
//
static ALWAYS inline int chn_send_(Channel* self, Scalar x)
{
	assert(!_chn_full(self));

	self->buffer[self->front] = x;
	self->front = (self->front+1) % self->size;

	assert(!_chn_empty(self));

	return thrd_success;
}

static ALWAYS inline int chn_receive(Channel* self, Scalar* x)
{
	assert(!_chn_empty(self));

	*x = self->buffer[self->back];
	self->back = (self->back+1) % self->size;

	assert(!_chn_full(self));

	return thrd_success;
}

#undef ALWAYS

#endif // CHANNEL_H

// vim:ai:sw=4:ts=4
