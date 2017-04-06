/**
 * @file init/xentium_demo.c
 *
 * @brief demonstrates how a Xentium processing network is used
 */

#include <kernel/kmem.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>

#include <asm-generic/io.h>

#include <modules-image.h>

#include <data_proc_net.h>
#include <kernel/xentium.h>



/**
 * Some implementation dependent op info passed by whatever created the task
 * this could also just exist in the <data> buffer as a interpretable structure.
 * This is really up to the user...
 * Note: the xentium kernel processing a task must know the same structure
 */

struct myopinfo {
	unsigned int ramplen;
};



/**
 * @brief the output function of the xentium processing network
 */

static int xen_op_output(unsigned long op_code, struct proc_task *t)
{
	ssize_t i;
	ssize_t n;

	unsigned int *p = NULL;



	/* need to address those caching issues at some point */
	asm("flush");

	n = pt_get_nmemb(t);



	if (!n)
		goto exit;



	p = (unsigned int *) pt_get_data(t);
	if (!p)
		goto exit;

	for (i = 0; i < n; i++)
		printk("XEN_OUT: \t%d\n", ioread32be(&p[i]));


exit:
	/* clean up our data buffer, its (de-)allocation is outside the
	 * responsibility of proc_task
	 */

	kfree(p);

	pt_destroy(t);

	return PN_TASK_SUCCESS;
}


/**
 * @brief create a task and add it to the Xentium processing network
 */

static void xen_new_input_task(size_t n)
{
	struct proc_task *t;

	static int seq;

	static int cnt;

	int i;
	unsigned int *data;

	struct myopinfo *nfo;

	nfo  = kzalloc(sizeof(struct myopinfo));
	if (!nfo)
		return;

	data = kzalloc(sizeof(unsigned int) * n);
	if (!data)
		return;

	for (i = 0; i < n; i++)
		data[i] = cnt++;

	t = pt_create(data, n, 6, 0, seq++);

	BUG_ON(!t);

	nfo->ramplen = 16;
	BUG_ON(pt_add_step(t, 0x00bada55, nfo));

	while (xentium_input_task(t) < 0)
		printk("Xenitium input busy!\n");
}



/**
 * @brief configure and run a xentium processing network demo
 */

void xen_demo(void)
{

	/* load all available Xentium kernels from the embedded modules image */
	module_load_xen_kernels();

	xentium_config_output_node(xen_op_output);

	xen_new_input_task(32);
	while (1) {

		xentium_output_tasks();
	}

}
