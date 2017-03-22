/**
 * @file lib/data_proc_tracker.c
 *
 *
 * This is a list based processing task reference tracker.
 *
 *
 */

#include <kernel/printk.h>
#include <kernel/kmem.h>
#include <kernel/log2.h>
#include <kernel/types.h>
#include <errno.h>

#include <data_proc_tracker.h>



/**
 * @brief returns the op code of the tracker
 *
 * @param pt a struct proc_tracker
 *
 * @returns the op code
 */

unsigned long pt_track_get_op_code(struct proc_tracker *pt)
{
	return pt->op_code;
}


/**
 * @brief check if the tracker is above its critical number of tasks
 *
 * @param pt a struct proc_tracker
 *
 * @returns 1 if critical, 0 if not
 */

int pt_track_level_critical(struct proc_tracker *pt)
{
	return (pt->n_tasks >= pt->n_tasks_crit);
}


/**
 * @brief returns the usage of the tracker
 *
 * @param pt a struct proc_tracker
 *
 * return number of tasks tracked
 */

int pt_track_get_usage(struct proc_tracker *pt)
{
	return pt->n_tasks;
}


/**
 * @brief check for pending tasks in a tracker
 *
 * @param pt a struct proc_tracker
 *
 * @returns 1 if tasks pending, 0 otherwise
 */

int pt_track_tasks_pending(struct proc_tracker *pt)
{
	if (pt && list_filled(&pt->tasks))
		return 1;

	return 0;
}


/**
 * @brief add a new item to the processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @param t a pointer to a task
 *
 * @returns 0 on success, -EINVAL on error
 *
 * @note if the pending step op code of the task does not match the op code of
 *	 the tracker, it is rejected
 */

int pt_track_put(struct proc_tracker *pt, struct proc_task *t)
{
	unsigned long op;


	if (!pt)
		return -EINVAL;

	if (!t)
		return -EINVAL;

	op = pt_get_pend_step_op_code(t);

	if (op != pt->op_code)
		return -1;

	list_add_tail(&t->node, &pt->tasks);

	pt->n_tasks++;

	return 0;
}


/**
 * @brief add a new item to the processing tracker, regardless of op code
 *
 * @param pt a struct processing_tracker
 *
 * @param t a pointer to a task
 *
 * @returns 0 on success, -EINVAL on error
 *
 */

int pt_track_put_force(struct proc_tracker *pt, struct proc_task *t)
{
	if (!pt)
		return -EINVAL;

	if (!t)
		return -EINVAL;

	list_add_tail(&t->node, &pt->tasks);

	pt->n_tasks++;

	return 0;
}


/**
 * @brief get the next item from processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @return processing task item or NULL if empty
 */

struct proc_task *pt_track_get(struct proc_tracker *pt)
{
	struct proc_task *t;

	if (!pt)
		return NULL;

	if (list_empty(&pt->tasks))
		return NULL;


	t = list_entry(pt->tasks.next, struct proc_task, node);

	list_del(&t->node);

	pt->n_tasks--;

	return t;
}



/**
 * @brief execute next item in processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @return -ENOEXEC if no execution (task list empty), otherwise return code of
 *	   associated op() function
 */

int pt_track_execute_next(struct proc_tracker *pt)
{
	struct proc_task *t;


	if (list_empty(&pt->tasks))
		return -ENOEXEC;


	t = list_entry(pt->tasks.next, struct proc_task, node);

	return pt->op(pt->op_code, t);
}




/**
 * @brief sort the tasks by order of sequence number
 * @param pt a struct processing_tracker
 *
 */

void pt_track_sort_seq(struct proc_tracker *pt)
{
	printk("TODO: list_sort not implemented\n");
}



/**
 * @brief create a processing tracker
 *
 * @param the function executing the op of this tracker
 * @param data optional data to pass to the the tracker
 * @param op_code the identfier of this tracker
 * @param tasks_crit the number of tasks after which the tracker is
 *	  considered filled to a critical level, must be at least 1
 *
 * @return processing tracker or NULL on error
 */

struct proc_tracker *pt_track_create(op_func_t op, unsigned long op_code,
				     size_t n_tasks_crit)
{
	struct proc_tracker *pt;


	if (!op)
		return NULL;

	if (!n_tasks_crit)
		return NULL;


	pt = (struct proc_tracker *) kzalloc(sizeof(struct proc_tracker));

	if (!pt)
		return NULL;

	pt->op = op;

	pt->n_tasks_crit = n_tasks_crit;

	pt->op_code = op_code;

	INIT_LIST_HEAD(&pt->tasks);

	return pt;
}


/**
 * @brief destroy a processing tracker and everything it tracks
 *
 * @param pt a struct processing_tracker
 */

void pt_track_destroy(struct proc_tracker *pt)
{
	struct proc_task *t;


	if (!pt)
		return;

	while (list_filled(&pt->tasks)) {
		t = pt_track_get(pt);

		pt_destroy(t);
	}

	kfree(pt);
}
