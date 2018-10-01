/* This demonstrates a basic configuration for and "embedded application"
 *
 * Note that at this point, we do not distinguish between an application
 * payload and a module, this will be changed in the future.
 */


#include "glue.h"




/* some random function we use in our executable */

static void print0r(int i)
{
	printk("print0r %d\n", i);
}

/* this is the equivalent of "main()" in our application
 * (can actually be main I guess)
 */

static int iasw(void *data)
{
	int i;

	(void) data;

	/* print a little */
	for (i = 0; i < 10; i++)
		print0r(i);

	/* then print a single "z" in a loop and "pause" until the function
	 * is re-scheduled by issuing sched_yield()
	 */

	while (1) {
		printk("z");
		sched_yield();
	}

	return 0;
}



/*
 * This is our startup call, it will be executed once when the application is
 * loaded. Here we create at thread in which our actual application code
 * will run in.
 */

int init_iasw(void)
{
	struct task_struct *t;

	t = kthread_create(iasw, (void *) 0, KTHREAD_CPU_AFFINITY_NONE, "IASW");
	kthread_wake_up(t);

	return 0;
}


/**
 * this is our optional exit call, it will be called when the module/application
 * is removed by the kernel
 */

int exit_iasw(void)
{
	printk("%s leaving\n", __func__);
	/* kthread_destroy() */
	return 0;
}

/* here we declare our init and exit functions */
module_init(init_iasw)
module_exit(exit_iasw)
