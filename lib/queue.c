/**
 * some helpers to build fifo queues
 */


#include <queue.h>

static void queue_inc_head(struct queue *q)
{
	q->head = (q->head + 1) & q->wrap;
}

static void queue_inc_tail(struct queue *q)
{
	q->tail = (q->tail + 1) & q->wrap;
}

size_t queue_empty(struct queue *q)
{
	if (!q)
		return 0;

	return q->head == q->tail;
}

size_t queue_used(struct queue *q)
{
	if (!q)
		return -1;

	return (q->head - q->tail) & q->wrap;
}

size_t queue_left(struct queue *q)
{
	if (!q)
		return 0;

	return (q->tail - q->head - 1) & q->wrap;
}

size_t queue_full(struct queue *q)
{
	if (!q)
		return 0;

	return !queue_left(q);
}

size_t queue_get(struct queue *q, char *c)
{
	if (!q)
		return 0;

	if (!c)
		return 0;

	if (!queue_used(q))
		return 0;


	(*c) = q->buf[q->tail];
	queue_inc_tail(q);

	return 1;
}

size_t queue_put(struct queue *q, char c)
{
	if (!q)
		return 0;

	if (!queue_left(q))
		return 0;

	q->buf[q->head] = c;
	queue_inc_head(q);

	return 1;
}
