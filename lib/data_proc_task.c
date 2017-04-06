/**
 * @file lib/data_proc_task.c
 *
 *
 * This manages processing tasks. Each task tracks an arbitrary number of steps
 * that each describe the next operation to perform on the data buffer tracked
 * by the processing task. Pending operations are stored in a "todo" list,
 * completed operations are stored in a "done" list.
 *
 * Each operation is described by "op_code" that encodes the
 * "processing operation" to be performed. Additional information may be
 * conveyed via the "op_info" pointer that the processing operation must
 * know how to interpret.
 *
 * The internal data buffer structure and size information must be interpretable
 * by the first processing operation.
 *
 * The data buffer need not be tracked in kmalloc() and will not be freed
 * on pt_destroy, but all op_info data are and hence must be allocated
 * accordingly.
 *
 * The number of elements may be any value that can be interpreted by the
 * prcessing task and need not represent the actual size of the buffer. (They
 * should not exceed the buffer size tho...)
 *
 * The "done" list of processing steps is currently only used to rewind a list
 * of processing steps, but may be used to generate a processing history later.
 * It is ordered from newest to oldest.
 *
 * Each process task may be assigned a type identifier for higher-level
 * tracking. Similarly, a sequence counter may be set when creating a processing
 * task, that can be used to restore a particular order, if for example multiple
 * tasks of the same type are merged into a new task and the output depends on
 * a particular sequence of results of the previous operations depend.
 *
 */


#include <kernel/printk.h>
#include <kernel/types.h>
#include <kernel/kmem.h>
#include <errno.h>


#include <data_proc_task.h>

#define MSG "PT: "


/**
 * @brief print the current processing todo list
 * @param t a struct proc_task
 */

void pt_dump_steps_todo(struct proc_task *t)
{
	struct proc_step *p_elem;


	printk(MSG "Dump of processing task TODO list [%p]\n"
	       MSG "\t[OP CODE]\t\t[OP INFO]\n", t);

	list_for_each_entry(p_elem, &t->todo, node)
		printk(MSG "\t%08x\t%p\n", p_elem->op_code, p_elem->op_info);

	printk(MSG "End of TODO list dump\n");
}


/**
 * @brief print the current processing done list
 * @param t a struct proc_task
 */

void pt_dump_steps_done(struct proc_task *t)
{
	struct proc_step *p_elem;


	printk(MSG "Dump of processing task DONE list [%p]\n"
	       MSG "\t[OP CODE]\t\t[OP INFO]\n", t);

	list_for_each_entry(p_elem, &t->done, node)
		printk(MSG "\t%08x\t%p\n", p_elem->op_code, p_elem->op_info);

	printk(MSG "End of DONE list dump\n");
}


/**
 * @brief move all processing steps from the done list to the todo list,
 *        restoring the original order
 *
 * @param t a struct proc_task
 */

void pt_rewind_steps_done(struct proc_task *t)
{
	struct proc_step *p_elem;
	struct proc_step *p_tmp;


	if (list_empty(&t->done))
		return;

	list_for_each_entry_safe(p_elem, p_tmp, &t->done, node)
		list_move(&p_elem->node, &t->todo);

}


/**
 * @brief delete the last done processing step and move it to free list
 *
 * @param t a struct proc_task
 *
 * @returns 0 or -1 if no more items in todo list
 */

int pt_del_last_step_done(struct proc_task *t)
{
	struct proc_step *s;


	if (list_empty(&t->done))
		return -1;

	s = list_entry(t->done.next, struct proc_step, node);

	list_move_tail(&s->node, &t->free);

	return 0;
}


/**
 * @brief delete pending processing step and move it to free list
 *
 * @param t a struct proc_task
 *
 * @returns 0 or -1 if no more items in todo list
 */

int pt_del_pend_step(struct proc_task *t)
{
	struct proc_step *s;


	if (list_empty(&t->todo))
		return -1;

	s = list_entry(t->todo.next, struct proc_step, node);

	list_move_tail(&s->node, &t->free);

	return 0;
}

/**
 * @brief move pending processing step to done list
 *
 * @param t a struct proc_task
 *
 * @returns 0 or -1 if no more items in todo list
 */

int pt_next_pend_step_done(struct proc_task *t)
{
	struct proc_step *s;


	if (list_empty(&t->todo))
		return -1;

	s = list_entry(t->todo.next, struct proc_step, node);

	list_move(&s->node, &t->done);

	return 0;
}


/**
 * @brief get the next pending processing step op code
 *
 * @param t a struct proc_task
 *
 * @returns op code or 0 if no more pending steps
 */

unsigned long pt_get_pend_step_op_code(struct proc_task *t)
{
	struct proc_step *s;

	if (!t)
		return 0;

	if (list_empty(&t->todo))
		return 0;

	s = list_entry(t->todo.next, struct proc_step, node);

	return s->op_code;
}


/**
 * @brief get the next pending processing step op info
 *
 * @param t a struct proc_task
 *
 * @returns op info pointer (may be NULL)
 */

void *pt_get_pend_step_op_info(struct proc_task *t)
{
	struct proc_step *s;


	if (list_empty(&t->todo))
		return NULL;

	s = list_entry(t->todo.next, struct proc_step, node);

	return s->op_info;
}


/**
 * @brief assign a processing step to the tail of the "todo" list
 *
 * @param t a struct proc_task
 * @param op_code the (nonzero) op code identifying the task
 * @param op_info a pointer to arbitrary data that is interpretable by the
 *	  operation processor
 *
 * @returns 0 on success,
 *	    -ENOMEM if out of free processing steps,
 *	    -EINVAL on invalid op code,
 */

int pt_add_step(struct proc_task *t,
		       unsigned long op_code, void *op_info)
{
	struct proc_step *s;

	if (!op_code)
		return -EINVAL;


	if (list_empty(&t->free))
		return -ENOMEM;


	s = list_entry(t->free.next, struct proc_step, node);

	list_del(&s->node);

	s->op_code = op_code;
	s->op_info = op_info;

	list_add_tail(&s->node, &t->todo);

	return 0;
}


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


/**
 * @brief get the type id of a processing task
 *
 * @param t a struct proc_task
 *
 * @returns the type id
 *
 */

unsigned long pt_get_type(struct proc_task *t)
{
	return t->type;
}


/**
 * @brief get the sequence number of a processing task
 *
 * @param t a struct proc_task
 *
 * @returns the sequence number
 *
 */

unsigned long pt_get_seq(struct proc_task *t)
{
	return t->seq;
}


/**
 * @brief set the number of elements in the data buffer of a processing task
 *
 * @param t a struct proc_task
 * @param nmemb the number of elements in the data buffer
 *
 * @return the number of elements in the buffer
 */

void pt_set_nmemb(struct proc_task *t, size_t nmemb)
{
	t->nmemb = nmemb;
}


/**
 * @brief set the data buffer in a processing task
 *
 * @param t a struct proc_task
 * @param data a pointer to a data buffer (may be NULL)
 * @param nmemb the number of elements in the buffer
 * @param steps the number of processing steps to allocate initially
 *
 * @return a pointer to the newly created task or NULL on error
 */

void pt_set_data(struct proc_task *t, void *data, size_t nmemb)
{
	t->data  = data;

	if (!t->data)
		t->nmemb = 0;
	else
		t->nmemb = nmemb;
}


/**
 * @brief set the type id of a processing task
 *
 * @param t a struct proc_task
 * @param type the type id to set
 *
 */

void pt_set_type(struct proc_task *t, unsigned long type)
{
	t->type = type;
}


/**
 * @brief set the sequence number of a processing task
 *
 * @param t a struct proc_task
 * @param seq the sequence number to set
 *
 */

void pt_set_seq(struct proc_task *t, unsigned long seq)
{
	t->seq = seq;
}


/**
 * @brief create a processing task
 *
 * @param data a pointer to a data buffer (may be NULL)
 * @param nmemb the number of elements in the buffer
 * @param steps the number of processing steps to allocate initially
 *
 * @param an arbitrary type identifier
 * @param an arbitrary sequence number
 *
 * @return a pointer to the newly created task or NULL on error
 */

struct proc_task *pt_create(void *data, size_t nmemb, size_t steps,
			    unsigned long type, unsigned long seq)
{
	size_t i;

	struct proc_task *t;


	t = (struct proc_task *) kzalloc(sizeof(struct proc_task));
	if (!t)
		return NULL;


	t->pool = (struct proc_step *) kzalloc(steps *
					       sizeof(struct proc_step));

	if (!t->pool) {
		kfree(t);
		return NULL;
	}

	INIT_LIST_HEAD(&t->todo);
	INIT_LIST_HEAD(&t->done);
	INIT_LIST_HEAD(&t->free);


	for (i = 0; i < steps; i++)
		list_add_tail(&t->pool[i].node, &t->free);


	t->data = data;
	t->type = type;
	t->seq  = seq;

	if (!t->data)
		t->nmemb = 0;
	else
		t->nmemb = nmemb;


	return t;
}


/**
 * @brief destroy a processing task
 *
 * @param t a struct proc_task
 *
 * @note this uses kfree() on "op_info", but "data" is untouched
 */

void pt_destroy(struct proc_task *t)
{
	struct proc_step *p_elem;


	if (!t)
		return;

	list_for_each_entry(p_elem, &t->todo, node)
		kfree(p_elem->op_info);

	list_for_each_entry(p_elem, &t->done, node)
		kfree(p_elem->op_info);

	list_for_each_entry(p_elem, &t->free, node)
		kfree(p_elem->op_info);

	kfree(t->pool);
	kfree(t);
}


