/**
 * @file include/queue.h
 *
 * @brief fixed-capacity ring queue implemented as macros
 *
 * Provides macros to declare, initialise and operate on a fixed-size ring
 * queue storing values of an arbitrary type by value.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <compiler.h>


/**
 * @brief define queue structure and forward-declare
 * @param name: the queue name
 * @param storage_type: the data type stored in the queue
 *
 * Creates a struct name_queue and an extern pointer named name.
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
 * @brief initialise the previously defined queue
 * @param name: the queue name
 * @param storage_type: the data type stored in the queue
 * @param storage_ptr: pointer to the backing storage array
 * @param n_elements: number of elements (must be a power of two)
 */
#define QUEUE_INIT(name, storage_type, storage_ptr, n_elements)						\
compile_time_assert(__builtin_popcount(n_elements) == 1, QUEUE_ELEMENTS_MUST_BE_A_POWER_OF_TWO);	\
struct name##_queue __##name = {0, 0, ((n_elements) - 1), (storage_ptr)};				\
struct name ## _queue *name = &__ ## name;


/* free elements left */
/**
 * @brief get the number of free elements remaining in the queue
 * @param queue: pointer to the queue
 * @return number of free slots
 */
#define queue_left(queue) \
	((queue->tail - queue->head - 1) & queue->wrap)

/**
 * @brief get the number of elements currently used in the queue
 * @param queue: pointer to the queue
 * @return number of used slots
 */
#define queue_used(queue) \
	((queue->head - queue->tail) & queue->wrap)

/**
 * @brief check whether the queue is empty
 * @param queue: pointer to the queue
 * @return non-zero if empty, 0 otherwise
 */
#define queue_empty(queue) \
	(queue->head == queue->tail)

/**
 * @brief check whether the queue is full
 * @param queue: pointer to the queue
 * @return non-zero if full, 0 otherwise
 */
#define queue_full(queue) \
	(!queue_left(queue))

/**
 * @brief get item from queue
 *
 * note: item_ptr will be dereferenced to store the item
 *
 * @param queue pointer to the queue
 * @param item_ptr pointer to a variable that receives the dequeued item
 *
 * @return 1 on success, 0 on error
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
 * @brief put item into queue
 *
 * note: item will be copied to queue
 *
 * @param queue pointer to the queue
 * @param item the item to enqueue
 *
 * @return 1 on success, 0 on error
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
 * @brief peek at an item in the queue
 *
 * note: item_ptr will be dereferenced to store the item
 * note: must queue_get() to actually remove the item
 *
 * @param queue pointer to the queue
 * @param item_ptr pointer to a variable that receives the peeked item
 *
 * @return 1 on success, 0 on error
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
