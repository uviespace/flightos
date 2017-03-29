
#include <xen.h>
#include <xen_printf.h>


#include <data_proc_net.h>

#define KERN_NAME		"xen_dummy"
#define KERN_STORAGE_BYTES	0
#define KERN_OP_CODE		0xdeadbeef
#define KERN_CRIT_TASK_LVL	25

/* actual configuration */
struct xen_kernel_cfg _xen_kernel_param = {
	KERN_NAME, KERN_OP_CODE,
	KERN_CRIT_TASK_LVL,
	NULL, KERN_STORAGE_BYTES,
};


static void process_task(struct xen_msg_data *m)
{
	size_t n;
	size_t i, j;

	unsigned int *p;


	if (!m->t) {
		m->cmd = TASK_DESTROY;
		return;
	}
#if 0
	/* op code doesn't match, pass it on */
	if (pt_get_pend_step_op_code(m->t) != _xen_kernel_param.op_code) {
		m->cmd = TASK_SUCCESS;
		return;
	}

#endif
	n = pt_get_nmemb(m->t);
	p = (unsigned int *) pt_get_data(m->t);

#if 0
	for (j = 0; j < 1000; j++) {
		for (i = 0; i < n; i++) {
			p[i] += 1230;
		}
	}
#endif
	m->cmd = TASK_SUCCESS;	
}



int main(void)
{
	struct xen_msg_data *m;


	while (1) {
		m = xen_wait_msg();

		switch (m->cmd) {
			case TASK_STOP:
				/* confirm abort */	
				x_printf("%s says bye!\n", _xen_kernel_param.name);
				xen_signal_host();
				return 0;
			default:
				break;
		}

		//x_printf("Hello there %s\n", _xen_kernel_param.name);
		process_task(m);

		xen_set_mail(XEN_MSG_MBOX, (unsigned long) m);
		xen_signal_host();
	}
	

	return 0;
}
