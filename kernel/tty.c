/**
 * @file kernel/tty.c
 *
 * @ingroup tty
 * @defgroup tty a TeleTYpewriter
 *
 * @brief something resembling a TTY
 *
 *
 * For now, we just collect any bytes from any thread, but we may want to
 * have an output channel per thread/process. I suppose we can solve the
 * buffering issue by having the process/thread initialise its own tty-channel,
 * providing the memory for the buffer. note that this would also
 * require the context to shut down the channel and deallocate.
 *
 */

#include <kernel/init.h>
#include <kernel/types.h>
#include <kernel/export.h>
#include <kernel/tty.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/printk.h>
#include <asm-generic/io.h>


/* if 0, unbuffered output is used (useful for crashes and debug message
 * printing
 */
#define ASYNC_OUTPUT 1



/* we can easily make queues more generic by setting buf to void
 * and adding an elem_size
 * OR we could just do everything a macros
 */

/* on init, we don't have kmalloc() */
#define TTY_BUF_SIZE 1024*4
static char buf[TTY_BUF_SIZE];
struct queue the_q = {0, 0, TTY_BUF_SIZE - 1, buf};


static void queue_inc_head(struct queue *q)
{
	q->head = (q->head + 1) & q->wrap;
}

static void queue_inc_tail(struct queue *q)
{
	q->tail = (q->tail + 1) & q->wrap;
}

size_t queue_empty(struct queue *q)
{
	if (!q)
		return 0;

	return q->head == q->tail;
}

size_t queue_used(struct queue *q)
{
	if (!q)
		return -1;

	return (q->head - q->tail) & q->wrap;
}

size_t queue_left(struct queue *q)
{
	if (!q)
		return 0;

	return (q->tail - q->head - 1) & q->wrap;
}

size_t queue_full(struct queue *q)
{
	if (!q)
		return 0;

	return !queue_left(q);
}

size_t queue_get(struct queue *q, char *c)
{
	if (!q)
		return 0;

	if (!c)
		return 0;

	if (!queue_used(q))
		return 0;


	(*c) = q->buf[q->tail];
	queue_inc_tail(q);

	return 1;
}

size_t queue_put(struct queue *q, char c)
{
	if (!q)
		return 0;

	if (!queue_left(q))
		return 0;

	q->buf[q->head] = c;
	queue_inc_head(q);

	return 1;
}


/**
 * XXX implement as architecture interface
 *
 * @returns bytes written or <0 on error
 *
 */
static int tty_tx(void *data)
{
	char c;

	struct queue *q = (struct queue *) data;

#define TX_EMPTY (1 << 2)
#define TX_FULL  (1 << 9)

#if defined(CONFIG_LEON3)
	static volatile int *console = (int *)0x80000100;
#endif

#if defined(CONFIG_LEON4)
	static volatile int *console = (int *)0xFF900000;
#endif

	while (1) {


		if (!(ioread32be(&console[1]) & TX_FULL)) {
			while (queue_get(q, &c)) {

				console[0] = c;

				if ((ioread32be(&console[1]) & TX_FULL))
					break;

			}
		}

		sched_yield();
	}

	return 0;
}

__attribute__((unused))
static int tty_write_internal(void *buf, size_t nbyte)
{
	size_t cnt = 0;

	char *c = buf;

	struct queue *q = &the_q;



	while (nbyte) {

		if (queue_full(q))
			break;

		if (c[cnt] == '\n') {
			/* need to append CR */
			if (queue_left(q) < 2)
				break;

			queue_put(q, '\n');
			queue_put(q, '\r');

		} else {
			queue_put(q, c[cnt]);
		}

		cnt++;
		nbyte--;
	}


	return cnt;
}
__attribute__((unused))
static int tty_write_internal_old(void *buf, size_t nbyte)
{
	char *c = buf;


#define TREADY 4

#if defined(CONFIG_LEON3)
       static volatile int *console = (int *)0x80000100;
#endif

#if defined(CONFIG_LEON4)
       static volatile int *console = (int *)0xFF900000;
#endif
       while (!(ioread32be(&console[1]) & TREADY));

       console[0] = 0x0ff & c[0];

       if (c[0] == '\n') {
               while (!(ioread32be(&console[1]) & TREADY));
               console[0] = (int) '\r';
	       }



	return 0;
}



int tty_write(void *buf, size_t nbyte)
{
#if ASYNC_OUTPUT
	tty_write_internal(buf, nbyte);
#else  /* in case of no printout */
	tty_write_internal_old(buf, nbyte);
#endif

	return 0;
}


/**
 * @brief initalises tty
 */

int tty_init(void)
{
	struct task_struct *t;

	t = kthread_create(tty_tx, &the_q, KTHREAD_CPU_AFFINITY_NONE, "TTY_TX");

#if ASYNC_OUTPUT
       	/* BUG: failure at kernel/sched/core.c:154/schedule()! after CPU1 boots */
      /* likely no problem, disabled BUG() call there */
	kthread_set_sched_edf(t, 10 * 1000, 9 * 1000, 50);
#endif
	if (kthread_wake_up(t) < 0) {
		printk("ALARM!\n");
		/* KERNEL ALARM */
	}

#if 0
	/* we keep the static initialisation as long as we don't have any cannels */
	the_q.buf = (char *) kmalloc(TTY_BUF_SIZE * sizeof(char));
	if (the_q.buf)
		the_q.wrap = TTY_BUF_SIZE - 1;
#endif

	return 0;
}
device_initcall(tty_init);

