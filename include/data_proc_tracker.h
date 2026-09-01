/**
 * @file include/data_proc_tracker.h
 *
 * @ingroup data_proc_tracker
 */

#ifndef _DATA_PROC_TRACKER_H_
#define _DATA_PROC_TRACKER_H_

#include <kernel/types.h>
#include <list.h>
#include <data_proc_task.h>

/**
 * @brief the operator function type associated with a processing tracker
 */
typedef int (*op_func_t)(unsigned long op_code, struct proc_task *);

/**
 * @brief the data processing tracker structure
 */
struct proc_tracker {
	struct list_head tasks;	/*!< a list head links the tracked tasks */ 
	size_t n_tasks;		/*!< the current number of tracked tasks */
	size_t n_tasks_crit;	/*!< the number of tasks above which the
				  tracker should be considered critical */

	unsigned long op_code;	/*!< the op code identifier of the tracker */

	op_func_t op;		/*!< the operator function of this tracker */


	struct list_head node;	/*!< may be used for external tracking of this
				   tracker */
};


/**
 * @brief get the op code identifier of a tracker
 * @param pt: the processing tracker
 * @return the tracker's op code
 */
unsigned long pt_track_get_id(struct proc_tracker *pt);

/**
 * @brief check whether a tracker has pending tasks
 * @param pt: the processing tracker
 * @return non-zero if tasks are pending, 0 otherwise
 */
int pt_track_tasks_pending(struct proc_tracker *pt);

/**
 * @brief get the current usage (task count) of a tracker
 * @param pt: the processing tracker
 * @return the number of tasks currently tracked
 */
int pt_track_get_usage(struct proc_tracker *pt);


/**
 * @brief check whether a tracker is at its critical task level
 * @param pt: the processing tracker
 * @return non-zero if at critical level, 0 otherwise
 */
int pt_track_level_critical(struct proc_tracker *pt);

/**
 * @brief add a task to a tracker queue if space is available
 * @param pt: the processing tracker
 * @param t: the task to add
 * @return 0 on success, negative error code on failure
 */
int pt_track_put(struct proc_tracker *pt, struct proc_task *t);

/**
 * @brief add a task to a tracker queue regardless of level (forced)
 * @param pt: the processing tracker
 * @param t: the task to add
 * @return 0 on success, negative error code on failure
 */
int pt_track_put_force(struct proc_tracker *pt, struct proc_task *t);

/**
 * @brief check whether a tracker has pending processing tasks
 * @param pt: the processing tracker
 * @return non-zero if pending tasks exist, 0 otherwise
 */
int pt_track_pending(struct proc_tracker *pt);

/**
 * @brief get (dequeue) the next pending task from a tracker
 * @param pt: the processing tracker
 * @return pointer to the dequeued task, or NULL if none available
 */
struct proc_task *pt_track_get(struct proc_tracker *pt);

/**
 * @brief sort the tracker's tasks by sequence number
 * @param pt: the processing tracker
 */
void pt_track_sort_seq(struct proc_tracker *pt);

/**
 * @brief create a new processing tracker
 * @param op: the operator function for this tracker
 * @param op_code: the op code identifier
 * @param n_tasks_crit: task count at which the tracker is considered critical
 * @return pointer to the new tracker, or NULL on failure
 */
struct proc_tracker *pt_track_create(op_func_t op, unsigned long op_code,
				     size_t n_tasks_crit);

/**
 * @brief destroy a processing tracker and release its resources
 * @param pt: the processing tracker to destroy
 */
void pt_track_destroy(struct proc_tracker *pt);


#endif /* _DATA_PROC_TRACKER_H_ */
