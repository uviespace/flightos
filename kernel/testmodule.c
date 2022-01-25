#include <kernel/module.h>
#include <kernel/kthread.h>
#include <kernel/sysctl.h>
#include <kernel/printk.h>

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


/**
 * this is the equivalent of "main()" in our module
 */

static int test(void *data)
{
	(void) data;


	/* print irq timers and yield until rescheduled */

	while (1) {
	//	show_timer_irq();
		sched_yield();
	}

	return 0;
}


/*
 * This is our startup call, it will be executed once when the module is
 * loaded. Here we create at thread in which our actual module code
 * will run in.
 */

int init_test(void)
{
	struct task_struct *t;

	t = kthread_create(test, NULL, KTHREAD_CPU_AFFINITY_NONE, "TEST");
#if 0
	/* allocate 2% of the cpu in a RT thread */
	kthread_set_sched_edf(t, 100*1000, 3*1000, 2*1000);
#else
	/* defauilt to RR */
#endif

	if (kthread_wake_up(t) < 0)
		printk("---- TEST NOT SCHEDULABLE---\n");

	return 0;
}


/**
 * this is our optional exit call, it will be called when the module/module
 * is removed by the kernel
 */

int exit_test(void)
{
	printk("module exit\n");
	/* kthread_destroy() */
	return 0;
}

/* here we declare our init and exit functions */
module_init(init_test);
module_exit(exit_test);
