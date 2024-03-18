#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <compiler.h>

#define QUEUE_DEFINE_INIT(name, storage_type, storage_ptr, n_elements) \
compile_time_assert(__builtin_popcount(n_elements) == 1, QUEUE_ELEMENTS_MUST_BE_A_POWER_OF_TWO); \
struct name##_queue { \
	size_t head; \
	size_t tail; \
	size_t wrap; \
	storage_type *data; \
} __ ## name = {0, 0, ((n_elements) - 1), (storage_ptr)}; \
struct name ## _queue *name = &__ ## name;


#define queue_left(queue) \
	((queue->tail - queue->head - 1) & queue->wrap)

#define queue_empty(queue) \
	(queue->head == queue->tail)

#define queue_used(queue) \
	((queue->head - queue->tail) & queue->wrap)

#define queue_full(queue) \
	(!queue_left(queue))

#define queue_get(queue, item_ptr) ({ \
	int __ret; \
	do { \
		if (queue == NULL) { \
			__ret = 0; \
		} else if(item_ptr == NULL) { \
			__ret = 0; \
		} else if(!queue_used(queue)) { \
			__ret = 0; \
		} else { \
			(*item_ptr) = queue->data[queue->tail]; \
			queue->tail = (queue->tail + 1) & queue->wrap; \
			__ret = 1; \
		}\
	} while(0); \
	__ret; \
	})

#define queue_put(queue, item) ({ \
	int __ret; \
	do { \
		if (queue == NULL) { \
			__ret = 0; \
		} else if(!queue_left(queue)) { \
			__ret = 0; \
		} else { \
			queue->data[queue->head] = item; \
			queue->head = (queue->head + 1) & queue->wrap; \
			__ret = 1; \
		}\
	} while(0); \
	__ret; \
	})

#endif /* QUEUE_H */
