/**
 * @file kernel/processing_tracker_circular.c
 *
 *
 * This is a processing task reference tracker, it does NOT make copies of the
 * items stored. The member sizes of a buffer are powers of two, size arguments
 * will be rounded up if necessary.
 *
 * @note deprecated in favour of list based tracker
 */

#include <kernel/printk.h>
#include <kernel/kmem.h>
#include <kernel/log2.h>
#include <kernel/types.h>
#include <errno.h>
#include <list.h>

#include <processing_task.h>



struct proc_tracker{
	struct proc_task **task;

	size_t rd;
	size_t wr;

	size_t n;
	size_t nmemb;
	size_t mask;
};


/**
 * @brief returns the usage of the tracker
 *
 * @param pt a struct proc_tracker
 *
 * return tracker usage in percent, range 0...100
 */

int pt_track_get_usage(struct proc_tracker *pt)
{
	return (pt->n * 100) / pt->nmemb;
}


/**
 * @brief add a new item to the processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @param t a pointer to a task
 *
 * @returns -1 if full
 */

int pt_track_put(struct proc_tracker *pt,
		 typeof((*((struct proc_tracker *)0)->task)) t)
{
	if (pt->n == pt->nmemb)
		return -1;

	pt->n++;
	pt->rd++;
	pt->rd &= pt->mask;

	pt->task[pt->rd] = t;


	return 0;
}


/**
 * @brief get an item from processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @return processing task item or NULL if empty
 */

typeof((*((struct proc_tracker *)0)->task))
pt_track_get(struct proc_tracker *pt)
{
	if (!pt->n)
		return NULL;

	pt->n--;
	pt->wr++;
	pt->wr &= pt->mask;

	return pt->task[pt->wr];
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

	pt->nmemb = roundup_pow_of_two(nmemb);
	pt->mask  = pt->nmemb - 1;

	pt->task = (typeof(pt->task)) kzalloc(pt->nmemb *
					      sizeof(typeof(pt->task)));

	if (!pt->task) {
		kfree(pt);
		return NULL;
	}


	return pt;
}


/**
 * @brief expand a processing tracker
 *
 * @param pt a struct processing_tracker
 *
 * @param nmemb the number of extra elements to track
 *
 * @return -1 on error, 0 on success
 */

int pt_track_expand(struct proc_tracker *pt, size_t nmemb)
{
	struct proc_tracker tmp;


	tmp.task = (typeof(pt->task)) krealloc(pt->task, (pt->nmemb + nmemb) *
					       sizeof(typeof(pt->task)));

	if (!tmp.task)
		return -1;


	pt->nmemb += nmemb;

	pt->mask = pt->nmemb - 1;

	pt->task = tmp.task;

	return 0;
}


/**
 * @brief destroy a processing tracker
 *
 * @param pt a struct processing_tracker
 */

void pt_track_destroy(struct proc_tracker *pt)
{
	kfree(pt->task);
	kfree(pt);
}
