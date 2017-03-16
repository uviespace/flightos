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
#include <list.h>

#include <data_proc_task.h>

struct proc_tracker {
	struct list_head tasks;
	size_t n;
};




/**
 * @brief returns the usage of the tracker
 *
 * @param pt a struct proc_tracker
 *
 * return number of tasks tracked 
 */

int pt_track_get_usage(struct proc_tracker *pt)
{
	return pt->n;
}


/**
 * @brief add a new item to the processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @param t a pointer to a task
 *
 * @returns 0 on success, -EINVAL on error
 */

int pt_track_put(struct proc_tracker *pt, struct proc_task *t)
{
	if (!pt)
		return -EINVAL;
	
	if (!t)
		return -EINVAL;

	list_add_tail(&t->node, &pt->tasks);

	pt->n++;

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


	if (list_empty(&pt->tasks))
		return NULL;

	
	t = list_entry(pt->tasks.next, struct proc_task, node);
	
	list_del(&t->node);
	
	pt->n--;

	return t;
}


/**
 * @brief sort the tasks by order of sequence number
 * @param pt a struct processing_tracker
 *
 */

void pt_track_sort_seq(struct proc_tracker *pt)
{
	printk("TODO: list_sort\n");
}



/**
 * @brief create a processing tracker
 *
 * @param nmemb the number of elements to track
 *
 * @return processing tracker or NULL on error
 */

struct proc_tracker *pt_track_create(size_t nmemb)
{
	struct proc_tracker *pt;


	pt = (struct proc_tracker *) kzalloc(sizeof(struct proc_tracker));

	if (!pt)
		return NULL;

	pt->n = 0;

	INIT_LIST_HEAD(&pt->tasks);

	return pt;
}


/**
 * @brief destroy a processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @return -1 if tracker is not empty, 0 on success
 */

int pt_track_destroy(struct proc_tracker *pt)
{
	if (list_filled(&pt->tasks))
		return -1;

	kfree(pt);

	return 0;
}
