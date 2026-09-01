/**
 * @file include/data_proc_task.h
 */

#ifndef _DATA_PROC_TASK_H_
#define _DATA_PROC_TASK_H_

#include <kernel/types.h>
#include <list.h>


/**
 * @brief a single processing step within a proc_task
 */
struct proc_step {

	unsigned long op_code;	/*!< the operation to perform on this item */
	void *op_info;		/*!< arbitrary additional data used by
				 * the processing of this item
				 */
	struct list_head node;
};

/**
 * @brief a data processing task
 */
struct proc_task {

	void *data;		/*!< the data buffer of this task */
	size_t size;		/*!< size of the buffer in bytes */
	size_t nmemb;		/*!< number of elements in the buffer */

	struct proc_step *pool;	/*!< pool of proc_step slots */

	struct list_head todo;	/*!< list of pending steps */
	struct list_head done;	/*!< list of completed steps */
	struct list_head free;	/*!< list of free steps */

	unsigned long type;	/*!< task type identifier */
	unsigned long seq;	/*!< sequence number */
	struct list_head node;	/*!< to be used for external tracking */
};


/**
 * @brief dump all pending (todo) steps of a task
 * @param t: the processing task
 */
void pt_dump_steps_todo(struct proc_task *t);

/**
 * @brief dump all completed (done) steps of a task
 * @param t: the processing task
 */
void pt_dump_steps_done(struct proc_task *t);

/**
 * @brief rewind all completed steps back to pending
 * @param t: the processing task
 */
void pt_rewind_steps_done(struct proc_task *t);

/**
 * @brief delete the last completed step
 * @param t: the processing task
 * @return 0 on success, negative error code on failure
 */
int pt_del_last_step_done(struct proc_task *t);

/**
 * @brief delete the first pending step
 * @param t: the processing task
 * @return 0 on success, negative error code on failure
 */
int pt_del_pend_step(struct proc_task *t);

/**
 * @brief delete all pending steps
 * @param t: the processing task
 */
void pt_del_all_pending(struct proc_task *t);

/**
 * @brief mark the next pending step as done
 * @param t: the processing task
 * @return 0 on success, negative error code on failure
 */
int pt_next_pend_step_done(struct proc_task *t);

/**
 * @brief get the op code of the next pending step
 * @param t: the processing task
 * @return the op code of the next pending step
 */
unsigned long pt_get_pend_step_op_code(struct proc_task *t);

/**
 * @brief get the op info of the next pending step
 * @param t: the processing task
 * @return pointer to the op info of the next pending step
 */
void *pt_get_pend_step_op_info(struct proc_task *t);

/**
 * @brief add a step to the pending list of a task
 * @param t: the processing task
 * @param op_code: the operation code for the step
 * @param op_info: additional data for the step (or NULL)
 * @return 0 on success, negative error code on failure
 */
int pt_add_step(struct proc_task *t,
		       unsigned long op_code, void *op_info);


/**
 * @brief get the number of elements in the task data buffer
 * @param t: the processing task
 * @return number of elements
 */
size_t pt_get_nmemb(struct proc_task *t);

/**
 * @brief get the size of the task data buffer
 * @param t: the processing task
 * @return buffer size in bytes
 */
size_t pt_get_size(struct proc_task *t);

/**
 * @brief get the task data buffer pointer
 * @param t: the processing task
 * @return pointer to the task data buffer
 */
void *pt_get_data(struct proc_task *t);

/**
 * @brief get the task type identifier
 * @param t: the processing task
 * @return the task type
 */
unsigned long pt_get_type(struct proc_task *t);

/**
 * @brief get the task sequence number
 * @param t: the processing task
 * @return the task sequence number
 */
unsigned long pt_get_seq(struct proc_task *t);

/**
 * @brief set the number of elements in the task data buffer
 * @param t: the processing task
 * @param nmemb: new element count
 */
void pt_set_nmemb(struct proc_task *t, size_t nmemb);

/**
 * @brief set the size of the task data buffer
 * @param t: the processing task
 * @param size: buffer size in bytes
 */
void pt_set_size(struct proc_task *t, size_t size);

/**
 * @brief set the task data buffer
 * @param t: the processing task
 * @param data: pointer to the new data buffer
 * @param size: size of the new data buffer in bytes
 */
void pt_set_data(struct proc_task *t, void *data, size_t size);

/**
 * @brief set the task type identifier
 * @param t: the processing task
 * @param type: the task type
 */
void pt_set_type(struct proc_task *t, unsigned long type);

/**
 * @brief set the task sequence number
 * @param t: the processing task
 * @param seq: the task sequence number
 */
void pt_set_seq(struct proc_task *t, unsigned long seq);


/**
 * @brief create a new processing task
 * @param data: pointer to the initial data buffer (or NULL)
 * @param size: size of the data buffer in bytes
 * @param steps: number of step slots to preallocate
 * @param type: task type identifier
 * @param seq: task sequence number
 * @return pointer to the new task, or NULL on failure
 */
struct proc_task *pt_create(void *data, size_t size, size_t steps,
			    unsigned long type, unsigned long seq);

/**
 * @brief destroy a processing task and release its resources
 * @param t: the processing task to destroy
 */
void pt_destroy(struct proc_task *t);



#endif /* _DATA_PROC_TASK_H_ */
