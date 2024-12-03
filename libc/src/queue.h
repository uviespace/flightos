#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <compiler.h>

__diag_ignore(GCC, 7, "-Wpedantic", "we WANT braced groups, this cannot be implemented otherwise. fuck -pedantic")

/**
 * define queue structure and forward-declare
 */
#define QUEUE_DECLARE(name, storage_type)	\
struct name##_queue {				\
	size_t head;				\
	size_t tail;				\
	size_t wrap;				\
	storage_type *data;			\
}; \
extern struct name ## _queue *name;


/**
 * initialise the previously defined queue
 */
#define QUEUE_INIT(name, storage_type, storage_ptr, n_elements)						\
compile_time_assert(__builtin_popcount(n_elements) == 1, QUEUE_ELEMENTS_MUST_BE_A_POWER_OF_TWO);	\
struct name##_queue __##name = {0, 0, ((n_elements) - 1), (storage_ptr)};				\
struct name ## _queue *name = &__ ## name;


/* free elements left */
#define queue_left(queue) \
	((queue->tail - queue->head - 1) & queue->wrap)

/* nuber of elements used */
#define queue_used(queue) \
	((queue->head - queue->tail) & queue->wrap)

/* empty check */
#define queue_empty(queue) \
	(queue->head == queue->tail)

/* full check */
#define queue_full(queue) \
	(!queue_left(queue))

/**
 * get item from queue
 * note: item_ptr will be dereferenced to store the item
 * returns 0 on error, otherwise success
 */
#define queue_get(queue, item_ptr) ({					\
	int __queue_get_ret;						\
	do {								\
		if (queue == NULL) {					\
			__queue_get_ret = 0;				\
		} else if(item_ptr == NULL) {				\
			__queue_get_ret = 0;				\
		} else if(!queue_used(queue)) {				\
			__queue_get_ret = 0;				\
		} else {						\
			(*item_ptr) = queue->data[queue->tail];		\
			queue->tail = (queue->tail + 1) & queue->wrap;	\
			__queue_get_ret = 1;				\
		}							\
	} while(0);							\
	(__queue_get_ret);						\
	})

/**
 * put item into queue
 * note: item will be copied to queue
 * returns 0 on error, otherwise success
 */
#define queue_put(queue, item) ({					\
	int __queue_put_ret;						\
	do {								\
		if (queue == NULL) {					\
			__queue_put_ret = 0;				\
		} else if(!queue_left(queue)) {				\
			__queue_put_ret = 0;				\
		} else {						\
			queue->data[queue->head] = item;		\
			queue->head = (queue->head + 1) & queue->wrap;	\
			__queue_put_ret = 1;				\
		}							\
	} while(0);							\
	(__queue_put_ret);						\
	})


/**
 * peek at an item in the queue
 * note: item_ptr will be dereferenced to store the item
 * returns 0 on error, otherwise success
 *
 * note: must queue_get() to actually remove the item
 */
#define queue_peek(queue, item_ptr) ({				\
	int __queue_peek_ret;					\
	do {							\
		if (queue == NULL) {				\
			__queue_peek_ret = 0;			\
		} else if(item_ptr == NULL) {			\
			__queue_peek_ret = 0;			\
		} else if(!queue_used(queue)) {			\
			__queue_peek_ret = 0;			\
		} else {					\
			(*item_ptr) = queue->data[queue->tail];	\
			__queue_peek_ret = 1;			\
		}						\
	} while(0);						\
	(__queue_peek_ret);					\
	})

#endif /* QUEUE_H */
