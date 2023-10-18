#ifdef DPU_TARGET__GLUE

#include <glue.h>

/**
 * @note at this point, we only get the kernel time (== system uptime)
 */

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	struct timespec ts;


	/* supress -Wunused */
	(void) clk_id;


	ts = get_ktime();

	tp->tv_sec  = ts.tv_sec;
	tp->tv_nsec = ts.tv_nsec;


	return 0;
}


/**
 * @note there is currently no support for timed sleep of a thread implemented
 *	 in the leanos kernel. To emulate the approximate behaviour, we simply
 *	 sched_yield() and will be woken after an arbitrary period when we are
 *	 scheduled again.
 */

int clock_nanosleep(clockid_t clock_id, int flags,
		    const struct timespec *rqtp, struct timespec *rmtp)
{
	/* supress -Wunused */
	(void) clock_id;
	(void) flags;
	(void) rqtp;
	(void) rmtp;

	sched_yield();

	return 0;
}


/**
 * @note this is a dummy; does not translate errno's to strings
 */

void perror(const char *s)
{
	printk("%s : some error occured\n", s);
}


void *malloc(size_t size)
{
	return kmalloc(size);
}


void free(void *ptr)
{
	kfree(ptr);
}


int printf(const char *fmt, ...)
{
	int ret;

	va_list args;

	va_start(args, fmt);
	ret = vprintf(fmt, args);
	va_end(args);

	return ret;
}

/* Someboedy is doing some uncontrolled byte swapping it seems.
 * I want this to just run right now, so I'll provide the function even though
 * there is most likely bad code somewhere
 */

int32_t __bswapsi2(int32_t a)
{
	return (int32_t) __bswap_32((uint32_t) a);
}


/** APPLICATION MODULE SETUP **/

/*
 * This is our startup call, it will be executed once when the application is
 * loaded. Here we create at thread in which our actual application code
 * will run in.
 */

int iasw_main(void *);

int init_iasw(void)
{
        struct task_struct *t;


        printk("%s entering\n", __func__);

        t = kthread_create(iasw_main, (void *) 0, KTHREAD_CPU_AFFINITY_NONE, "IASW");
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


#endif /* DPU_TARGET__GLUE */
