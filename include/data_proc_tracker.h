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
 * the operator function type associated with a processing tracker
 */
typedef int (*op_func_t)(unsigned long op_code, struct proc_task *);

/**
 * @struct the data processing tracker structure
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


unsigned long pt_track_get_id(struct proc_tracker *pt);

int pt_track_tasks_pending(struct proc_tracker *pt);

int pt_track_get_usage(struct proc_tracker *pt);


int pt_track_level_critical(struct proc_tracker *pt);

int pt_track_put(struct proc_tracker *pt, struct proc_task *t);

int pt_track_put_force(struct proc_tracker *pt, struct proc_task *t);

int pt_track_pending(struct proc_tracker *pt);

struct proc_task *pt_track_get(struct proc_tracker *pt);

void pt_track_sort_seq(struct proc_tracker *pt);

struct proc_tracker *pt_track_create(op_func_t op, unsigned long op_code,
				     size_t n_tasks_crit);

void pt_track_destroy(struct proc_tracker *pt);


#endif /* _DATA_PROC_TRACKER_H_ */
