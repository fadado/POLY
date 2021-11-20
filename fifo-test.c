// Channel test

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// comment next line to enable assertions
//#define NDEBUG
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// Scalar
////////////////////////////////////////////////////////////////////////

typedef union {
	int64_t  i;
	uint64_t u;
	double   r;
	void*    p;
} Scalar __attribute__((__transparent_union__));

////////////////////////////////////////////////////////////////////////
// Channel
////////////////////////////////////////////////////////////////////////

#define N 10

typedef struct {
	size_t front;
	size_t rear;
	size_t size;
	Scalar buffer[N+1]; // TODO: use malloc
} Channel;

#define ALWAYS __attribute__((always_inline))

static inline void chn_init(Channel* self, size_t size);
static inline void chn_destroy(Channel* self);
static ALWAYS inline bool chn_empty(Channel* self);
static ALWAYS inline bool chn_full(Channel* self);
static ALWAYS inline void chn_snd_(Channel* self, Scalar x);
static ALWAYS inline void chn_rcv(Channel* self, Scalar* x);
//#define chn_cast(scalar,variable) 

// Channel life

static inline void chn_init(Channel* self, size_t size)
{
	self->front = self->rear = 0;
	self->size = size+1;
	// TODO: malloc
	assert(chn_empty(self));
	assert(!chn_full(self));
}

static inline void chn_destroy(Channel* self)
{
	; // TODO
}

// Predicates

static ALWAYS inline bool chn_empty(Channel* self)
{
	return self->front == self->rear;
}

static ALWAYS inline bool chn_full(Channel* self)
{
	return ((self->front+1) % self->size) == self->rear;
}

// Send & Receive

static ALWAYS inline void chn_snd_(Channel* self, Scalar x)
{
	assert(!chn_full(self));
	self->buffer[self->front] = x;
	self->front = (self->front+1) % self->size;
	assert(!chn_empty(self));
}

#define chn_snd(C,S) chn_snd_((C), _Generic((S),\
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
	default: (void*)(intptr_t)(S))/* assume pointer */\
)

static ALWAYS inline void chn_rcv(Channel* self, Scalar* x)
{
	assert(!chn_empty(self));
	*x = self->buffer[self->rear];
	self->rear = (self->rear+1) % self->size;
	assert(!chn_full(self));
}

#define chn_cast(S,V) _Generic(V,\
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
	default: (void*)(S).p\
)

#undef ALWAYS

////////////////////////////////////////////////////////////////////////
// TEST
////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	static_assert(sizeof(double)==sizeof(int64_t));
	static_assert(sizeof(double)==sizeof(uint64_t));
	static_assert(sizeof(double)==sizeof(void*));
	static_assert(sizeof(Scalar)==8);

	Channel chn;

	chn_init(&chn, N);

	for (char c='0'; !chn_full(&chn); ++c) {
		chn_snd(&chn, c);
	}
	assert(chn_full(&chn));

	Scalar s;
	for (char c; !chn_empty(&chn);) {
		chn_rcv(&chn, &s);
		c = chn_cast(s, c); // chn_set(c, s); ???
		putchar(c);
	}
	assert(chn_empty(&chn));

	putchar('\n');

	chn_destroy(&chn);

	return 0;
}

// vim:ai:sw=4:ts=4
