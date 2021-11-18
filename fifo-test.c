// FIFO test

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

// comment next line to enable assertions
//#define NDEBUG
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// FIFO
////////////////////////////////////////////////////////////////////////

#define E int
#define N 10

typedef struct {
	size_t front;
	size_t rear;
	size_t size;
	E      buffer[N+1];
} FIFO;

#define ALWAYS __attribute__((always_inline))

static ALWAYS inline bool q_empty(FIFO* self)
{
	return self->front == self->rear;
}

static ALWAYS inline bool q_full(FIFO* self)
{
	return ((self->front+1) % self->size) == self->rear;
}

static inline void q_init(FIFO* self)
{
	self->front = self->rear = 0;
	self->size = sizeof(self->buffer) / sizeof(*self->buffer);
	assert(q_empty(self));
	assert(!q_full(self));
}

static inline void q_put(FIFO* self, E x)
{
	assert(!q_full(self));
	self->buffer[self->front] = x;
	self->front = (self->front+1) % self->size;
	assert(!q_empty(self));
}

static inline void q_get(FIFO* self, E* x)
{
	assert(!q_empty(self));
	*x = self->buffer[self->rear];
	self->rear = (self->rear+1) % self->size;
	assert(!q_full(self));
}

////////////////////////////////////////////////////////////////////////
// TEST
////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	FIFO queue;
	int i;

	q_init(&queue);

	for (i='0'; !q_full(&queue); ++i) {
		q_put(&queue, i);
	}
	assert(q_full(&queue));

	while (!q_empty(&queue)) {
		q_get(&queue, &i);
		putchar(i);
	}
	assert(q_empty(&queue));
	putchar('\n');

	return 0;
}

// vim:ai:sw=4:ts=4
