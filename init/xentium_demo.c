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
 * @brief the output function of the xentium processing network
 */

static int xen_op_output(unsigned long op_code, struct proc_task *t)
{
	ssize_t i;
	ssize_t n;

	unsigned int *p = NULL;



	n = pt_get_nmemb(t);



	if (!n)
		goto exit;



	p = (unsigned int *) pt_get_data(t);
	if (!p)
		goto exit;

	printk("XEN_OUT: \t%d\n", ioread32be(&p[n-1]));


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



	data = kzalloc(sizeof(unsigned int) * n);
	if (!data)
		return;

	for (i = 0; i < n; i++)
		data[i] = cnt++;

	t = pt_create(data, n, 6, 0, seq++);

	BUG_ON(!t);

	BUG_ON(pt_add_step(t, 0xdeadbeef, NULL));
	BUG_ON(pt_add_step(t, 0xb19b00b5, NULL));
	BUG_ON(pt_add_step(t, 0xdeadbeef, NULL));
	BUG_ON(pt_add_step(t, 0xb19b00b5, NULL));
	BUG_ON(pt_add_step(t, 0xb19b00b5, NULL));

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

	while (1) {
		static int seq = 100;

		if (seq < 120)
			xen_new_input_task(seq++);
		
		xentium_output_tasks();
	}

}
