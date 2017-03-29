
#include <data_proc_task.h>


/* the xentium must be able to interpret the proc task
 * XXX this is just a hack, do it properly */




/**
 * @brief get the number of elements in the data buffer of a processing task
 *
 * @param t a struct proc_task
 *
 * @return the number of elements in the buffer
 */

size_t pt_get_nmemb(struct proc_task *t)
{
	return t->nmemb;
}

/**
 * @brief get the data buffer in a processing task
 *
 * @param t a struct proc_task
 *
 * @return the pointer to a data buffer (may be NULL)
 */

void *pt_get_data(struct proc_task *t)
{
	return t->data;
}
