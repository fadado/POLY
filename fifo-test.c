// Channel test

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

// comment next line to enable assertions
//#define NDEBUG
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// Channel of scalars
////////////////////////////////////////////////////////////////////////

typedef union {
	int64_t  i;
	uint64_t u;
	double   r;
	void*    p;
} Scalar __attribute__((__transparent_union__));

typedef struct {
	size_t  front;
	size_t  rear;
	size_t  size;
	Scalar* buffer;
} Channel;

static int  chn_init(Channel* self, size_t capacity);
static void chn_destroy(Channel* self);
static bool chn_empty(Channel* self);
static bool chn_full(Channel* self);
static void chn_send_(Channel* self, Scalar x);
static void chn_receive(Channel* self, Scalar* x);

// Cast an Scalar to the variable type
#define chn_cast(S,V) _Generic((V),\
	_Bool: (_Bool)(S).u,\
	char: (char)(S).u,\
	signed char: (signed char)(S).i,\
	unsigned char: (unsigned char)(S).u,\
	signed short int: (signed short int)(S).i,\
	unsigned short int: (unsigned short int)(S).u,\
	signed int: (signed int)(S).i,\
	unsigned int: (unsigned int)(S).u,\
	signed long int: (signed long int)(S).i,\
	unsigned long int: (unsigned long int)(S).u,\
	signed long long int: (signed long long int)(S).i,\
	unsigned long long int: (unsigned long long int)(S).u,\
	float: (float)(S).r,\
	double: (double)(S).r,\
	long double: (long double)(S).r,\
	default: (void*)(S).p)

#define ALWAYS __attribute__((always_inline))

//
// Channel life
//
static inline int chn_init(Channel* self, size_t capacity)
{
	self->front = self->rear = 0;
	self->size  = capacity+1;

	self->buffer = calloc(self->size, sizeof(Scalar));
	if (!self->buffer) return thrd_nomem;

	assert(chn_empty(self));
	assert(!chn_full(self));
	assert(self->buffer);

	return thrd_success;
}

static inline void chn_destroy(Channel* self)
{
	free(self->buffer);
	self->front = self->rear = self->size = 0;
	self->buffer = (Scalar*)0;
}

//
// Predicates
//
static ALWAYS inline bool chn_empty(Channel* self)
{
	return self->front == self->rear;
}

static ALWAYS inline bool chn_full(Channel* self)
{
	return ((self->front+1) % self->size) == self->rear;
}

//
// Send & Receive
//
static ALWAYS inline void chn_send_(Channel* self, Scalar x)
{
	assert(!chn_full(self));

	self->buffer[self->front] = x;
	self->front = (self->front+1) % self->size;

	assert(!chn_empty(self));
}

#define chn_send(C,S) chn_send_((C), _Generic((S),\
	_Bool: (uint64_t)(S),\
	char: (uint64_t)(S),\
	signed char: (int64_t)(S),\
	unsigned char: (uint64_t)(S),\
	signed short int: (int64_t)(S),\
	unsigned short int: (uint64_t)(S),\
	signed int: (int64_t)(S),\
	unsigned int: (uint64_t)(S),\
	signed long int: (int64_t)(S),\
	unsigned long int: (uint64_t)(S),\
	signed long long int: (int64_t)(S),\
	unsigned long long int: (uint64_t)(S),\
	float: (double)(S),\
	double: (double)(S),\
	long double: (double)(S),\
	default: (void*)(intptr_t)(S))/*ASSUME POINTER*/\
)

static ALWAYS inline void chn_receive(Channel* self, Scalar* x)
{
	assert(!chn_empty(self));

	*x = self->buffer[self->rear];
	self->rear = (self->rear+1) % self->size;

	assert(!chn_full(self));
}

#undef ALWAYS

////////////////////////////////////////////////////////////////////////
// TEST
////////////////////////////////////////////////////////////////////////

#define N 10

int main(int argc, char* argv[])
{
	static_assert(sizeof(double)==sizeof(int64_t));
	static_assert(sizeof(double)==sizeof(uint64_t));
	static_assert(sizeof(double)==sizeof(void*));
	static_assert(sizeof(Scalar)==8);

	int err;
#	define catch(e)	if ((err=(e)) != thrd_success) goto onerror

	Channel chn;
	catch(chn_init(&chn, N));

	for (char c='0'; !chn_full(&chn); ++c) {
		chn_send(&chn, c);
	}
	assert(chn_full(&chn));

	Scalar s;
	for (char c; !chn_empty(&chn);) {
		chn_receive(&chn, &s);
		c = chn_cast(s, c);
		putchar(c);
	}
	assert(chn_empty(&chn));

	putchar('\n');

	chn_destroy(&chn);

	return EXIT_SUCCESS;

onerror:
#	undef catch
	switch (err) {
		case thrd_nomem:
			fprintf(stderr, "%s\n", strerror(ENOMEM));
			break;
		case thrd_busy: // TODO
		case thrd_error:
		case thrd_timedout:
		default: abort();
	}
	return EXIT_FAILURE;
}

// vim:ai:sw=4:ts=4
