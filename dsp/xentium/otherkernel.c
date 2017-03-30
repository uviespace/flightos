#include <xen.h>
#include <dma.h>
#include <xen_printf.h>
#include <data_proc_net.h>

#define KERN_NAME		"otherkernel"
#define KERN_STORAGE_BYTES	0
#define KERN_OP_CODE		0xb19b00b5
#define KERN_CRIT_TASK_LVL	12

/* actual configuration */
struct xen_kernel_cfg _xen_kernel_param  __attribute__((used)) = {
	KERN_NAME, KERN_OP_CODE,
	KERN_CRIT_TASK_LVL,
	NULL, KERN_STORAGE_BYTES,
};


static void process_task(struct xen_msg_data *m)
{
	size_t n;
	size_t i;

	unsigned int *p;
	volatile unsigned int *b;

	struct xen_tcm *tcm_ext;

	
	b = (volatile unsigned int *) xen_tcm_local;

	tcm_ext = xen_get_base_addr(m->xen_id);


	if (!m->t) {
		m->cmd = TASK_DESTROY;
		return;
	}
	

	n = pt_get_nmemb(m->t);
	p = (unsigned int *) pt_get_data(m->t);

	/* retrieve data to TCM XXX retval */
	xen_noc_dma_req_lin_xfer(m->dma, p, tcm_ext, n, WORD, LOW, 256);

	/* process */
	for (i = 0; i < n; i++) {
		b[i] *= 2;
	}

	/* back to main memory XXX retval */
	xen_noc_dma_req_lin_xfer(m->dma, tcm_ext, p, n, WORD, LOW, 256);

	m->cmd = TASK_SUCCESS;
}



int main(void)
{
	struct xen_msg_data *m;

	while (1) {
		m = xen_wait_cmd();

		switch (m->cmd) {
			case TASK_STOP:
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
