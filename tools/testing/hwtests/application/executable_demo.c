/* This demonstrates a basic configuration for and "embedded application"
 *
 * Note that at this point, we do not distinguish between an application
 * payload and a module, this will be changed in the future.
 *
 * NOTE: this is configured for the GR740
 */


#include "glue.h"

__attribute__((unused))
static int show_timer_irq(void)
{
	char buf0[64];
	char buf1[64];
	char buf2[64];
	char buf3[64];
	char buf4[64];

	struct sysobj *sys_irq = NULL;


	sys_irq = sysset_find_obj(sys_set, "/sys/irl/primary");

	if (!sys_irq) {
		printk("Error locating sysctl entry\n");
		return -1;
	}

	while (1) {

		sysobj_show_attr(sys_irq, "irl", buf0);
		sysobj_show_attr(sys_irq, "1",   buf1);
		sysobj_show_attr(sys_irq, "2",   buf2);
		sysobj_show_attr(sys_irq, "3",   buf3);
		sysobj_show_attr(sys_irq, "4",   buf4);

		printk("IRQ total: %s\n"
		       "CPU timers: \n"
		       " [0]: %s\n"
		       " [1]: %s\n"
		       " [2]: %s\n"
		       " [3]: %s\n",
		       buf0, buf1, buf2, buf3, buf4);

		sched_yield();
	}

	return 0;
}


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
		printk("%d\n", i++);
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

	t = kthread_create(iasw, NULL, KTHREAD_CPU_AFFINITY_NONE, "IASW");
	/* allocate 98% of the cpu */
	kthread_set_sched_edf(t, 100*1000, 99*1000, 98*1000);
	if (kthread_wake_up(t) < 0)
		printk("---- IASW NOT SCHEDULABLE---\n");

	return 0;
}


/**
 * this is our optional exit call, it will be called when the module/application
 * is removed by the kernel
 */

int exit_iasw(void)
{
	printk("program leaving\n");
	/* kthread_destroy() */
	return 0;
}

/* here we declare our init and exit functions */
module_init(init_iasw)
module_exit(exit_iasw)
