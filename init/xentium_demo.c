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
#include <xentium_demo.h>



/**
 * @brief the output function of the xentium processing network
 */

static int xen_op_output(unsigned long op_code, struct proc_task *t)
{
	size_t i;
	size_t n;

	unsigned int *p = NULL;


	/* need to address those caching issues at some point */
	ioread32be(&t);

	n = pt_get_nmemb(t);
	p = pt_get_data(t);

	if (n && p) {
		for (i = 0; i < n; i++)
			printk("XEN_OUT: \t%d\n", ioread32be(&p[i]));
	} else {
		printk("XEN_OUT: releasing empty task\n");
	}


	/* clean up our data buffer, its (de-)allocation is outside the
	 * responsibility of proc_task
	 */

	kfree(p);

	pt_destroy(t);

	return PN_TASK_SUCCESS;
}


/**
 * @brief create a task and add it to the Xentium processing network
 *
 * There is no particular configuration in mind, all kernels were selected
 * simply to demonstrate the features available.
 * see dsp/xentium/kernels for the kernel used
 */

static void xen_new_input_task(size_t n)
{
	struct proc_task *t;

	static int seq;

	static int cnt;

	int i;
	unsigned int *data;

	struct deglitch_op_info *deglitch_nfo;
	struct stack_op_info *stack_nfo;
	struct ramp_op_info *ramp_nfo;


	/* Kernels may need information on how to process an item. We allocate
	 * these structures below
	 */

	deglitch_nfo  = kzalloc(sizeof(struct deglitch_op_info));
	if (!deglitch_nfo)
		return;

	stack_nfo  = kzalloc(sizeof(struct stack_op_info));
	if (!stack_nfo)
		return;

	ramp_nfo  = kzalloc(sizeof(struct ramp_op_info));
	if (!ramp_nfo)
		return;

	/* Allocate a work buffer. Typically, this would be a custom allocator
	 * provided by the Xentium processing network driver and located in
	 * a suitable memory bank, e.g. NoC SDRAM. For the purpose of this demo,
	 * we simply use the kernel memory manager to allocate the buffer
	 */

	data = kzalloc(sizeof(unsigned int) * n);
	if (!data)
		return;

	/* initialise with a value */
	for (i = 0; i < n; i++)
		data[i] = cnt++;

	/* create a processing task that holds at most processing steps
	 * and assign it a work buffer, it's size and a sequence number
	 */
	t = pt_create(data, n * sizeof(unsigned int), 6, 0, seq++);
	BUG_ON(!t);

	/* Set the number of elements placed in the work buffer. Note
	 * that this is not necessarily identical to the size of the buffer
	 * (but the total size of elements should never exceed the buffer
	 * storage, obviously)
	 */
	pt_set_nmemb(t, n);

	/* median filter */
	data[19] += 3000;	/* a "random" glitch */
	data[20] += 10000;
	data[21] += 8000;
	deglitch_nfo->sigclip = 3;
	/* add a step with the kernel op code 0xbadc0ded and additional
	 * information for the operation
	 */
	BUG_ON(pt_add_step(t, 0xbadc0ded, deglitch_nfo));

	/* random dummy action, kernel does nothing but pass through data */
	BUG_ON(pt_add_step(t, 0xdeadbeef, NULL));

	/* data stacking */
	stack_nfo->stackframes = 4;
	BUG_ON(pt_add_step(t, 0x1fedbeef, stack_nfo));

	/* ramp fit */
	ramp_nfo->ramplen = 32;
	BUG_ON(pt_add_step(t, 0x00bada55, ramp_nfo));

	/* add task top processing network */
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

	/* set the user-defined output node */
	xentium_config_output_node(xen_op_output);

	/* our "stackframes" parameter in our demo is 4, so we'll create
	 * quadruples of tasks of the same number of data points
	 */

	xen_new_input_task(32);
	xen_new_input_task(32);
	xen_new_input_task(32);
	xen_new_input_task(32);

	xen_new_input_task(64);
	xen_new_input_task(64);
	xen_new_input_task(64);
	xen_new_input_task(64);

	xen_new_input_task(64);
	xen_new_input_task(64);
	xen_new_input_task(64);
	xen_new_input_task(64);


	/* Process the network outputs in an infinite loop. Since we have
	 * 12 inputs, a stacking of 4 and a ramp length of 32, we will
	 * get 1 data point output for inputs of 32 items and 2 for inputs
	 * of 64 items, i.e. 5 in total.
	 */

	while (1)
		xentium_output_tasks();

}
