/**
 *
 * kmalloc/kfree via host processor for added convenience...
 *
 */


#include <xen.h>
#include <kernel/kmem.h>

	
/* make sure this exists in actual memory, i.e. in .bss */
static struct xen_msg_data msg_data;






/**
 * @brief perform memory allocation via host processor
 *
 * @todo no error checking/handling
 */

void *kzalloc(size_t size)
{
	struct xen_msg_data *m = &msg_data;

	if (!size)
		return NULL;

	m->size = size;
	m->cmd = TASK_KZALLOC;

	xen_send_msg(m);
	
	m = xen_wait_cmd();

	return m->ptr;
}


/**
 * @brief perform memory allocation via host processor
 *
 * @todo no error checking/handling
 */

void *kmalloc(size_t size)
{
	struct xen_msg_data *m = &msg_data;

	if (!size)
		return NULL;

	m->size = size;
	m->cmd = TASK_KMALLOC;

	xen_send_msg(m);
	
	m = xen_wait_cmd();

	return m->ptr;
}


/**
 * @brief perform memory deallocation via host processor
 *
 * @todo no error checking/handling
 */

void kfree(void *ptr)
{
	struct xen_msg_data *m = &msg_data;


	if (!ptr)
		return;

	m->ptr = ptr;
	m->cmd = TASK_KFREE;

	xen_send_msg(m);
	
	xen_wait_cmd();

	return;
}
