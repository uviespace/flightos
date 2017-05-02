/**
 * @ingroup xen_kernel
 *
 * @brief deglitch an array
 *
 * NOTE: This is for demonstration purposes only. There are lot of things that
 * are not verified/handled/you name it, but the purpose of this kernel
 * is to show off the what can be done and how with the resources available.
 */


#include <xen.h>
#include <dma.h>
#include <xen_printf.h>
#include <data_proc_net.h>
#include <kernel/kmem.h>
#include <xentium_demo.h>

#define DMA_MTU	256	/* arbitrary DMA packet size */

/* this kernel's properties */
#define KERN_NAME		"deglitch"
#define KERN_STORAGE_BYTES	0
#define KERN_OP_CODE		0xbadc0ded
#define KERN_CRIT_TASK_LVL	5

struct xen_kernel_cfg _xen_kernel_param = {
	KERN_NAME, KERN_OP_CODE,
	KERN_CRIT_TASK_LVL,
	NULL, KERN_STORAGE_BYTES,
};


/**
 * @brief determine the median of 3 values
 */

static int median3(int a, int b, int c)
{
        int t;

        if (a > b) {
                t = a;
                a = b;
                b = t;

        } else if (b > c) {
                t = b;
                b = c;
                c = t;

        } else if (a > b) {
                t = a;
                a = b;
                b = t;
        }

        return b;
}

/**
 * @brief get the absolute of a value
 */

static int abs(int a)
{
	if (a >= 0)
		return a;

	return -a;
}

/**
 * @brief apply a median filter of lenght 3 to data
 *
 * @param in  the input array
 * @param out the output array
 * @param len the number of elements in the array
 * @param threshold the input value above which to apply the filter
 */

static void median3filter(volatile int *in, int *out, size_t len, unsigned int threshold)
{
	int i, m;


	out[0] = in[0];

	for (i = 1; i < (len-1); i++) {

		m = median3(in[i-1], in[i], in[i+1]);

		if (abs(m - in[i]) > threshold)
			out[i] = m;
		else
			out[i] = in[i];
	}

	out[len-1] = in[len-1];
}


/**
 * @brief calculate an integer square root
 * @note from wikipedia (I think)
 */

static int isqrt(int num)
{

	int res = 0;
	int bit = 1 << 30;


	while (bit > num)
		bit >>= 2;

	while (bit) {

		if (num >= res + bit) {
			num -= res + bit;
			res = (res >> 1) + bit;
		} else {
			res >>= 1;
		}

		bit >>= 2;
	}

	return res;
}


/**
 * @brief calculate sigma of integer array
 *
 * @returns sigma as integer
 */
__attribute__((unused))
static int get_int_sigma(volatile int *in, size_t len)
{
	size_t i;

	int tmp;

	int mean = 0;
	unsigned int variance = 0;



	/* for size of datatype and complexity reasons we will use
	 * variance = E(X - E(X))^2
	 */

	for (i = 0; i < len; i++)
		mean += in[i];

	mean /= len;


	for (i = 0; i < len; i++) {
		tmp = in[i] - mean;
		variance += tmp * tmp;
	}


	return isqrt(variance / len);
}


/**
 * @brief get average residual in L1 norm E(abs(X - E(X)))
 *
 * this works better than non-iterative clipping
 */

static int get_int_L1_residual(volatile int *in, size_t len)
{
	size_t i;

	int tmp;
	int mean = 0;
/**
 * Some implementation dependent op info passed by whatever created the task
 * this could also just exist in the <data> buffer as a interpretable structure.
 * This is really up to the user...
 * Note: the xentium kernel processing a task must know the same structure
 */
	unsigned int residual = 0;


	for (i = 0; i < len; i++)
		mean += in[i];

	mean /= len;

	for (i = 0; i < len; i++) {
		tmp = (in[i] - mean);
		residual += abs(tmp);
	}


	return residual / len;
}


/**
 * @brief apply a clipped median filter to an array
 *
 */

static void median_filter_cipped(volatile int *in, int *out, size_t len, unsigned int clip)
{
	unsigned int threshold;


	/* this is not iterative */
#if 0
	threshold = clip * get_int_sigma(in, len);
#else
	threshold = clip * get_int_L1_residual(in, len);
#endif

	median3filter(in, out, len, threshold);
}


/**
 * here we do the work
 */

static void process_task(struct xen_msg_data *m)
{
	size_t n;
	size_t len;

	int *p;

	volatile int *b1;
	volatile int *b3;

	struct xen_tcm *tcm_ext;

	struct xen_tcm *tcm_ext_out;

	struct deglitch_op_info *op_info;



	if (!m->t) {
		m->cmd = TASK_DESTROY;
		return;
	}

	/* These refers to the TCM banks. Note that at least b1 must be
	 * volatile, or clang soils itself because the local TCM starts at 0x0
	 * and (in this case incorrectly) detects that as a NULL pointer
	 * dereference.
	 */

	b1 = (volatile int *) xen_tcm_local->bank1;
	b3 = (volatile int *) xen_tcm_local->bank3;


	/* determine our TCM's external address, so we can program DMA
	 * transfers correctly
	 */
	tcm_ext = xen_get_base_addr(m->xen_id);

	/* lazily set the external address of our data output here */
	tcm_ext_out = (void *) ((char *) xen_get_base_addr(m->xen_id)
				+ XEN_TCM_BANK_SIZE * 2);


	op_info = pt_get_pend_step_op_info(m->t);
	if (!op_info) {
		m->cmd = TASK_DESTROY;
		return;
	}

	/* size of the data buffer */
	len = pt_get_size(m->t);

	/* number of elements to process */
	n = pt_get_nmemb(m->t);

	if (n > len / sizeof(int)) {
		printk("realloc of data buffer not implemented, clipping");
		n = len / sizeof(int);
	}

	if (n > (2 * XEN_TCM_BANK_SIZE / sizeof(int))) {
		printk("Warning: median filtering for n > %d not implemented, "
		       "clipping.\n", 2 * XEN_TCM_BANK_SIZE / sizeof(int));

		n = 2 * XEN_TCM_BANK_SIZE / sizeof(int);
	}


	/* the data buffer of this task */
	p = (int *) pt_get_data(m->t);

	/* retrieve data to TCM */
	xen_noc_dma_req_lin_xfer(m->dma, p, tcm_ext, n, WORD, LOW, DMA_MTU);


	/* process input in banks 1 & 2 into output banks 3 & 4
	 * larger inputs are not implemented at this time
	 */
	median_filter_cipped(b1, (int *) b3, n, op_info->sigclip);

	/* copy result back to data buffer */
	xen_noc_dma_req_lin_xfer(m->dma, tcm_ext_out, p, n, WORD, LOW, DMA_MTU);

	/* number of elements did not change */

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
