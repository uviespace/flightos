/**
 * @brief stack "frames"
 *
 * NOTE: This is for demonstration purposes only. There are lot of things that
 * are not verified/handled/you name it, but the purpose of this kernel
 * is to show off the what can be done and how with the resources available.
 *
 * This includes:
 *	- detaching of tasks into kernel storage and stacking their data
 *
 *
 * Note: When locally storing the task references in a list rather than
 *	 linearly, they can be sorted/removed more efficiently, in case there
 *	 is a dependence on the sequence number of the tasks before stacking.
 */


#include <xen.h>
#include <dma.h>
#include <xen_printf.h>
#include <data_proc_net.h>
#include <kernel/kmem.h>
#include <xentium_demo.h>

/* track at most 4 tasks across calls to this kernel */
#define TASKS_MAX	4


/* this kernel's properties */
#define KERN_NAME		"stack"
#define KERN_STORAGE_BYTES	(TASKS_MAX * sizeof(struct proc_task *))
#define KERN_OP_CODE		0x1fedbeef
#define KERN_CRIT_TASK_LVL	5

struct xen_kernel_cfg _xen_kernel_param = {
	KERN_NAME, KERN_OP_CODE,
	KERN_CRIT_TASK_LVL,
	NULL, KERN_STORAGE_BYTES,
};



/**
 * here we do the work
 */

static void process_task(struct xen_msg_data *m)
{
	size_t n;
	size_t i, j;

	size_t tracked = 0;

	int *p, *pp;

	struct stack_op_info *op_info;

	struct proc_task *t;
	struct proc_task **tasks = NULL;



	if (!_xen_kernel_param.data) {
		printk("No permanent-storage memory configured, bailing.\n");
		m->cmd = TASK_DESTROY;
		return;
	}

	tasks = _xen_kernel_param.data;

	while (tasks[tracked])
		tracked++;



	if (!m->t) {
		m->cmd = TASK_DESTROY;
		return;
	}


	op_info = pt_get_pend_step_op_info(m->t);
	XBUG();
	if (!op_info) {
		m->cmd = TASK_DESTROY;
		return;
	}

	if (op_info->stackframes > TASKS_MAX) {
		printk("Can at most track %d tasks, but %d were requested. "
		       "Bailing.\n", TASKS_MAX, op_info->stackframes);
		m->cmd = TASK_DESTROY;
		return;
	};

	/* let's see if we need more task */
	if (tracked < op_info->stackframes - 1) {
		tasks[tracked] = m->t;
		m->cmd = TASK_DETACH;
		return;
	}

	/* let's blindly assume that all have the same number of elements :D */
	n = pt_get_nmemb(m->t);

	/* the data buffer of this task */
	p = (int *) pt_get_data(m->t);


	for (i = 0; i < tracked; i++) {

		t = tasks[i];

		/* get one of the tasks in our storage */
		pp = (int *) pt_get_data(t);

		/* and stack it to our current (not using the DMA, I'm lazy) */
		for (j = 0; j < n; j++)
			p[j] += pp[j];

		/* remove all pending steps in this task and set its content
		 * to zero, so it will be directly moved to the output node as
		 * we reattach it to the hosts processing network
		 */
		pt_set_nmemb(t, 0);
		pt_del_all_pending(t);
	}

	/* store our current task temporarily */
	t = m->t;

	/* now mop up */
	for (i = 0; i < tracked; i++) {
		m->t = tasks[i];
		m->cmd = TASK_ATTACH;

		/* hand it back to the the host */
		xen_send_msg(m);

		/* its response is just TASK_ATTACH as confirmation*/
		xen_wait_cmd();

		/* remove it from our local storage */
		tasks[i] = NULL;
	}

	/* hand back the stacked data to the host for further processing */
	m->t = t;

	/* and we're done */
	m->cmd = TASK_SUCCESS;
}


/**
 * the main function
 */

int main(void)
{
	struct xen_msg_data *m;


	while (1) {
		m = xen_wait_cmd();

		if (!m) {
			printk("Invalid command location, bailing.");
			return 0;
		}


		switch (m->cmd) {
		case TASK_EXIT:
			/* confirm abort */
			xen_send_msg(m);
			return 0;
		default:
			break;
		}

		process_task(m);

		xen_send_msg(m);
	}

	return 0;
}
