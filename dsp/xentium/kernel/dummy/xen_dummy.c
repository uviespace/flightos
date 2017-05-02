/**
 * @ingroup xen_kernel
 *
 * @brief this is just a dummy that uses the DMA to copy at most one TCM
 *	  bank of 32 bit words from the main memory and back
 */

#include <xen.h>
#include <dma.h>
#include <xen_printf.h>
#include <data_proc_net.h>

#define DMA_MTU	256	/* arbitrary DMA packet size */

#define KERN_NAME		"xen_dummy"
#define KERN_STORAGE_BYTES	0
#define KERN_OP_CODE		0xdeadbeef
#define KERN_CRIT_TASK_LVL	0xdead

/* actual configuration */
struct xen_kernel_cfg _xen_kernel_param = {
	KERN_NAME, KERN_OP_CODE,
	KERN_CRIT_TASK_LVL,
	NULL, KERN_STORAGE_BYTES,
};



static void process_task(struct xen_msg_data *m)
{
	size_t n;

	unsigned int *p;
	volatile unsigned int *b1;

	struct xen_tcm *tcm_ext;


	b1 = (volatile unsigned int *) xen_tcm_local;



	if (!m->t) {
		m->cmd = TASK_DESTROY;
		return;
	}

	tcm_ext = xen_get_base_addr(m->xen_id);


	n = pt_get_nmemb(m->t);
	p = pt_get_data(m->t);


	if (n > XEN_TCM_BANK_SIZE / sizeof(unsigned int))
		n = XEN_TCM_BANK_SIZE / sizeof(unsigned int);

	/* retrieve data to TCM */
	xen_noc_dma_req_lin_xfer(m->dma, p, tcm_ext, n, WORD, LOW, DMA_MTU);

	/* no processing */

	/* back to main memory  */
	xen_noc_dma_req_lin_xfer(m->dma, tcm_ext, p, n, WORD, LOW, DMA_MTU);


	m->cmd = TASK_SUCCESS;
}



int main(void)
{
	struct xen_msg_data *m;

	while (1) {
		m = xen_wait_cmd();

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
