#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <kselftest.h>

#include <shared.h>
#include <kmalloc_test_wrapper.h>
#include <wrap_ktime_get.h>

#include <kernel/kthread.h>

#ifdef printk
#undef printk
#define printk(fmt, ...) printk(fmt, ...)
#endif


/* include header + src file for static function testing */
//#include <kernel/sysctl.h>
#include <sched/edf.c>


static ktime kernel_time;
static unsigned long tick_period_min_ns = 100000UL;

/* needed dummy functions */
unsigned long tick_get_period_min_ns(void)
{
        return tick_period_min_ns;
}

int sched_register(struct scheduler *sched)
{
}


/* tests */


/*
 * @test sched_edf_init_test
 */

static void sched_edf_init_test(void)
{
	KSFT_ASSERT(sched_edf_init() == 0);
}


/*
 * @test sched_edf_create_tasks_test
 */

static void sched_edf_create_tasks_test(void)
{
	struct task_struct *t;
	struct sched_attr attr;


	/* create task 1 */
	t = kmalloc(sizeof(struct task_struct));
	KSFT_ASSERT_PTR_NOT_NULL(t);

	t->name = kmalloc(32);
	KSFT_ASSERT_PTR_NOT_NULL(t->name);

	snprintf(t->name, 32, "task_1");

	t->sched = &sched_edf;
	t->attr.policy       = SCHED_EDF;
	t->attr.period       = us_to_ktime(1000);
	t->attr.deadline_rel = us_to_ktime(900);
	t->attr.wcet         = us_to_ktime(255);
	edf_enqueue(&t->sched->tq, t);


	/* create task 2 */
	t = kmalloc(sizeof(struct task_struct));
	KSFT_ASSERT_PTR_NOT_NULL(t);

	t->name = kmalloc(32);
	KSFT_ASSERT_PTR_NOT_NULL(t->name);

	snprintf(t->name, 32, "task_2");

	t->sched = &sched_edf;
	t->attr.policy       = SCHED_EDF;
	t->attr.period       = us_to_ktime(1500);
	t->attr.deadline_rel = us_to_ktime(400);
	t->attr.wcet         = us_to_ktime(300);
	edf_enqueue(&t->sched->tq, t);


	/* create task 3 */
	t = kmalloc(sizeof(struct task_struct));
	KSFT_ASSERT_PTR_NOT_NULL(t);

	t->name = kmalloc(32);
	KSFT_ASSERT_PTR_NOT_NULL(t->name);

	snprintf(t->name, 32, "task_3");

	t->sched = &sched_edf;
	t->attr.policy       = SCHED_EDF;
	t->attr.period       = us_to_ktime(30);
	t->attr.deadline_rel = us_to_ktime(20);
	t->attr.wcet         = us_to_ktime(10);
	edf_enqueue(&t->sched->tq, t);


	/* create task 4 */
	t = kmalloc(sizeof(struct task_struct));
	KSFT_ASSERT_PTR_NOT_NULL(t);

	t->name = kmalloc(32);
	KSFT_ASSERT_PTR_NOT_NULL(t->name);

	snprintf(t->name, 32, "task_4");

	t->sched = &sched_edf;
	t->attr.policy       = SCHED_EDF;
	t->attr.period       = us_to_ktime(3000);
	t->attr.deadline_rel = us_to_ktime(900);
	t->attr.wcet         = us_to_ktime(100);
	edf_enqueue(&t->sched->tq, t);
}


/*
 * @test sched_edf_create_tasks_test
 */

#define CYCLES 10000

static void sched_edf_schedule_test(void)
{
	int i;
	int64_t wake;
	int64_t slice;

	struct task_struct *next = NULL;
	struct task_struct *curr = NULL;



	for (i = 0; i < CYCLES; i++) {
		curr = next;

		if (curr) {
			printk("started: %lld now %lld\n", curr->exec_start, ktime_get());
			/* remove runtime of slice from curr */
			curr->runtime = ktime_sub(curr->runtime, ktime_sub(ktime_get(), curr->exec_start));
		}

		edf_wake_next(&sched_edf.tq);

		sched_print_edf_list_internal(&sched_edf.tq, ktime_get());

		next = edf_pick_next(&sched_edf.tq);
		sched_print_edf_list_internal(&sched_edf.tq, ktime_get());

		if (next) {
			slice = next->runtime;
			printk("Next: %s slice %lld\n", next->name, ktime_to_us(slice));
		} else {
			slice = 1000000000; /* retry in 1 second */
			printk("Next: NONE\n");
		}

		wake = edf_task_ready_ns(&sched_edf.tq);
		printk("New task ready in %llu\n", ktime_to_us(wake));

		if (wake < slice) {
			printk("reducing slice from %lld to %lld (%lld)\n", ktime_to_us(slice), ktime_to_us(wake), ktime_to_us(wake - slice));
			slice = wake;
		}


		/* prepare run: save slice start time */
		if (next)
			next->exec_start = ktime_get();



		/* our timeslice has passed: assume we return in time */
		kernel_time += slice;
		printk("\npretending slice of %lld\n", ktime_to_us(slice));
		ktime_wrap_set_time(kernel_time);

	}
}



int main(int argc, char **argv)
{

	printk("Testing EDF scheduler\n\n");

	/* we need full control over ktime */
	ktime_get_wrapper(ENABLED);

	KSFT_RUN_TEST("sched_edf_init",
		      sched_edf_init_test);

	KSFT_RUN_TEST("creating tasks",
		      sched_edf_create_tasks_test)

	KSFT_RUN_TEST("emulating scheduling cycles",
		      sched_edf_schedule_test)


	printk("\n\nEDF scheduler test complete:\n");

	ksft_print_cnts();

	return ksft_exit_pass();
}
