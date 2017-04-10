/**
 * @brief fit ramps to data in an array
 *
 * NOTE: This is for demonstration purposes only. There are lot of things that
 * are not verified/handled/you name it, but the purpose of this kernel
 * is to show off the what can be done and how with the resources available.
 * This includes:
 *	- command exchange with the host processor
 *	- access to processing tasks
 *	- complex DMA transfer
 *	- kmalloc/kfree via the host processor
 *	- integration of custom (really seariously very superfast) assembly for
 *	  the actual processing
 *
 * NOTE: the number of samples per ramp must be a multiple of 4
 */


#include <xen.h>
#include <dma.h>
#include <xen_printf.h>
#include <data_proc_net.h>
#include <kernel/kmem.h>
#include <xentium_demo.h>

#define DMA_MTU	256	/* arbitrary DMA packet size */

/* this kernel's properties */

#define KERN_NAME		"rampfit"
#define KERN_STORAGE_BYTES	0
#define KERN_OP_CODE		0x00bada55
#define KERN_CRIT_TASK_LVL	25

struct xen_kernel_cfg _xen_kernel_param = {
	KERN_NAME, KERN_OP_CODE,
	KERN_CRIT_TASK_LVL,
	NULL, KERN_STORAGE_BYTES,
};



/* prototype of our assembly function */
int FastIntFixedRampFitBuffer(long *bank1, long *bank2,
			      unsigned int n_samples, unsigned int ramplen,
			      long *slopes);

/**
 * here we do the work
 */

static void process_task(struct xen_msg_data *m)
{
	size_t n;
	size_t len;
	size_t n_ramps;

	long *p;
	long *slopes;

	volatile long *b1;
	volatile long *b2;
	volatile long *b3;

	struct xen_tcm *tcm_ext;

	struct ramp_op_info *op_info;


	if (!m->t) {
		m->cmd = TASK_DESTROY;
		return;
	}

	/* These refers to the TCM banks. Note that at least b1 must be
	 * volatile, or clang soils itself because the local TCM starts at 0x0
	 * and (in this case incorrectly) detects that as a NULL pointer
	 * dereference.
	 */

	b1 = (volatile long *) xen_tcm_local->bank1;
	b2 = (volatile long *) xen_tcm_local->bank2;
	b3 = (volatile long *) xen_tcm_local->bank3;


	/* determine our TCM's external address, so we can program DMA
	 * transfers correctly
	 */
	tcm_ext = xen_get_base_addr(m->xen_id);

	op_info = pt_get_pend_step_op_info(m->t);
	if (!op_info) {
		m->cmd = TASK_DESTROY;
		return;
	}

	/* lenght of the data buffer */
	len = pt_get_size(m->t);

	/* number of elements to process */
	n = pt_get_nmemb(m->t);

	if (n > len / sizeof(long)) {
		n = len / sizeof(long);
	}

	if (n & 0x3) {
		printk("Warning: N is not a multiple of 4, adjusting.\n");
		n &= ~0x3;
	}


	/* The buffer to store the slopes may be anywhere in memory. We only
	 * write to it every couple of cycles, no problem.
	 * (Don't do this if you want fast accesses/reads!)
	 */
	n_ramps = n / op_info->ramplen;
	slopes = kzalloc(n_ramps * sizeof(long));


	/* the data buffer of this task */
	p = (long *) pt_get_data(m->t);

	/* retrieve data to TCM
	 * XXX no retval handling
	 * NOTE: we support at most 8k 32 bit samples for one processing round
	 * where the number of samples are (rounded down) to a multiple of four,
	 * i.e. one can fit at most 2048 ramps of 4 samples per "task"
	 * (remember this was made for demonstrational purposes only)
	 *
	 *
	 * How this is done: the ramp fit assembly implementation we use here
	 * expects us to deliver the data in two parallel TCM banks, which
	 * should be banks 1 and 3, which in principle allows us to work on
	 * ramps of up to 8192 samples total, which must be stored in two bank
	 * groups sequentially but "de-interleaved", so there are no same-bank
	 * access conflicts of the Xentiums E units and we may load 4 samples at
	 * in every clock cycle:
	 *
	 *  +----------v
	 *  ⎮    1     2     3     4
	 *  ⎮ +-----+-----+-----+-----+
	 *  ⎮ ⎮  1  ⎮  9  ⎮  2  ⎮ 10  ⎮
	 *  ⎮ ⎮  3  ⎮ ... ⎮  4  ⎮ ... ⎮
	 *  ⎮ ⎮ ... ⎮ ... ⎮ ... ⎮ ... ⎮
	 *  ⎮ ⎮  7  ⎮ 21  ⎮  8  ⎮ 22  ⎮
	 *  ⎮ +-----+-----+-----+-----+
	 *  +____v
	 *
	 *
	 *
	 * To set this up, we'll perform a DMA transfer to split the linear
	 * sequences of ramp data into the banks.
	 *
	 * We instruct the DMA to consider the data in a shape that is 2 columns
	 * by n/2 rows. Iterating over the source, it will take items from
	 * columns sequentially and place the into the target with an offset of
	 * 2 bank sizes apart (b3 - b1). It will then forward-skip two columns
	 * to reach the next row, but perform no skip at the target, because
	 * there the column shape is 1 column by n/2 rows.
	 */

	xen_noc_dma_req_xfer(m->dma, p, tcm_ext, 2, n/2, WORD,
			     1, (int16_t)(b3 - b1),
			     2, 1, LOW, DMA_MTU);

	/* process the ramps*/
	n_ramps = FastIntFixedRampFitBuffer((long *) b1,
				  (long *) b3,
				  n, op_info->ramplen, slopes);

	/* Now copy the resulting slopes to the data buffer and free the
	 * temporary allocation.
	 * We do this by performing two transfers, one from the slopes buffer to
	 * the TCM, then back to the data buffer of the task.
	 * We have to do this, because the DMA cannot perform transfers into the
	 * same NoC node (if you do, it gets stuck)
	 */
	xen_noc_dma_req_lin_xfer(m->dma, slopes, tcm_ext, n_ramps,
				 WORD, LOW, DMA_MTU);

	/* now into the task_'s data buffer */
	xen_noc_dma_req_lin_xfer(m->dma, tcm_ext, p, n_ramps,  WORD, LOW, DMA_MTU);

	/* update the member size of the buffer */
	pt_set_nmemb(m->t, n_ramps);

	/* free the slope buffer */
	kfree(slopes);

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
