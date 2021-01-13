
#include <kernel/kernel.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/err.h>
#include <kernel/smp.h>
#include <asm/io.h>

/* test duration */
#define SECONDS	10

/* copied buffer elements */
#define BUFLEN	1024*1024

static volatile double per_loop_avg[CONFIG_SMP_CPUS_MAX];

static int copytask(void *data)
{
	int i;
	int cpu;
	int *go;

	ktime cnt = 0;
	ktime start, stop;
	ktime total = 0;

//	static uint32_t *common[CONFIG_SMP_CPUS_MAX];
	static uint32_t *cpu_buf[CONFIG_SMP_CPUS_MAX];



	go = (int *) data;


	cpu = smp_cpu_id();

	cpu_buf[smp_cpu_id()] = kmalloc(BUFLEN * sizeof(uint32_t));


	if (!cpu_buf[cpu])
		return 0;

	(*go) = 1;	/* signal ready */

	/* wait for trigger */
	while (ioread32be(go) != CONFIG_SMP_CPUS_MAX);

	while (ioread32be(go)) {
		start = ktime_get();

		for (i = 0 ; i < BUFLEN; i++) {
			cpu_buf[cpu][i] = cpu_buf[CONFIG_SMP_CPUS_MAX - cpu - 1][i];
		}

		stop = ktime_get();

		total += stop - start;

		cnt++;

		per_loop_avg[cpu] = ( ((double) total / (double) cnt) / (double) (BUFLEN));

	}

	return 0;
}

int copy_resprint(void *data)
{
	int i;
	int *go;

	ktime start;

	double res[CONFIG_SMP_CPUS_MAX];


	go = (int *) data;

	/* wait for trigger */
	while (ioread32be(go) != CONFIG_SMP_CPUS_MAX);

	start = ktime_get();

	printk("Time [s] |  CPU(s) [ns / sample] \n");
	/* wait for about 60 seconds */
	while (ktime_delta(ktime_get(), start) < ms_to_ktime(SECONDS * 1000)) {


		for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++)
			res[i] = per_loop_avg[i];

		printk("%g ", 0.001 * (double) ktime_to_ms(ktime_get()));

		for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++)
			printk("%g ", res[i]);

		printk("\n");
	}


	(*go) = 0; /* signal stop */

	return 0;
}


int copybench_start(void)
{
	int i;
	int go;

	struct task_struct *t;



	printk("COPYBENCH STARTING\n");

	printk("Creating tasks, please stand by\n");

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {
//	for (i = CONFIG_SMP_CPUS_MAX - 1; i >= 0; i--) {

		go = 0;

		t = kthread_create(copytask, &go, i, "COPYTASK");

		if (!IS_ERR(t)) {
			/* allocate 95% of the cpu, period = 1s */
			kthread_set_sched_edf(t, 1000 * 1000, 980 * 1000, 950 * 1000);

			if (kthread_wake_up(t) < 0) {
				printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
				BUG();
			}

			while (!ioread32be(&go)); /* wait for task to become ready */

		} else {
			printk("Got an error in kthread_create!");
			break;
		}

		printk("Copy task ready on cpu %d\n", i);
	}

	printk("Creating RR cpu-hopping printout task\n");

	t = kthread_create(copy_resprint, &go, KTHREAD_CPU_AFFINITY_NONE, "PRINTTASK");
	if (kthread_wake_up(t) < 0) {
		printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
		BUG();
	}

	printk("Triggering...\n");

	go = CONFIG_SMP_CPUS_MAX; /* set trigger */
	sched_yield();

	while (ioread32be(&go)); /* wait for completion */

	printk("Average time to cross-copy buffers:\n");

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {
		printk("\tCPU %d: %g ns per sample\n", i, per_loop_avg[i]);
	}

	printk("COPYBENCH DONE\n");

	return 0;
}


int edftask(void *data)
{
	int i;
	int loops = (* (int *) data);


	for (i = 0; i < loops; i++);


	return i;
}


int oneshotedf_start(void)
{
	int i;
	int loops = 1700000000;

	struct task_struct *t[CONFIG_SMP_CPUS_MAX];



//	printk("EDF CREATE STARTING\n");

//	printk("Creating tasks, please stand by\n");

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {

		t[i] = kthread_create(edftask, &loops, i, "EDFTASK");

		if (!IS_ERR(t)) {
			/* creaate and launch edf thread */
			kthread_set_sched_edf(t[i], 0, 100, 50);

			if (kthread_wake_up(t[i]) < 0) {
				printk("---- %s NOT SCHEDUL-ABLE---\n", t[i]->name);
				BUG();
			}
		} else {
			printk("Got an error in kthread_create!");
			break;
		}

//		printk("Copy task ready on cpu %d\n", i);
	}

	sched_yield();

	printk("%lld\n", ktime_to_ms(ktime_get()));
	printk("Wakeup, creation, exec_start, exec_stop, deadline:\n");

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {
		printk("\tCPU %d: %lld %lld %lld %lld %lld\n",
		       i,
		       ktime_to_us(ktime_delta(t[i]->wakeup_first,     t[i]->create)),
		       ktime_to_us(ktime_delta(t[i]->wakeup,     t[i]->create)),
		       ktime_to_us(ktime_delta(t[i]->exec_start, t[i]->create)),
		       ktime_to_us(ktime_delta(t[i]->exec_stop, t[i]->create)),
		       ktime_to_us(ktime_delta(t[i]->deadline,   t[i]->create)));
	}

	printk("oneshot_edf DONE\n");

	return 0;


}
