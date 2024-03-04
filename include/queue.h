/**
 * @file include/queue.h
 *
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>


struct queue {
	size_t head;
	size_t tail;
	size_t wrap;
	char *buf;
};

size_t queue_left(struct queue *q);
size_t queue_empty(struct queue *q);
size_t queue_used(struct queue *q);
size_t queue_left(struct queue *q);
size_t queue_full(struct queue *q);
size_t queue_get(struct queue *q, char *c);
size_t queue_put(struct queue *q, char c);

#endif /* QUEUE_H */
