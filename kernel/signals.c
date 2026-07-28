#include <kernel/signals.h>
#include <errno.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/string.h>
#include <kernel/smp.h>
#include <kernel/syscall.h>
#include <asm/processor.h>
#include <asm/spinlock.h>
#include <asm-generic/irqflags.h>

int syscall_sched_yield(void);
int syscall_schedule(void);


#define MSG "KSIGNAL: "


struct ksig_reg {
	int signal;
	struct list_head tasks;
	struct list_head node;
};

static LIST_HEAD(ksignals);

static int ksig_cnt;
static struct spinlock ksig_spinlock;


static void ksig_lock(void)
{
	spin_lock_raw(&ksig_spinlock);
}


static void ksig_unlock(void)
{
	spin_unlock(&ksig_spinlock);
}

static void ksig_dec(int count)
{
	ksig_cnt -= count;
}


static void ksig_inc(void)
{
	ksig_cnt++;
}

static void ksig_tsk_sig_cnt_dec(struct task_struct *tsk)
{

}


static int ksig_exec(void *data)
{
	struct task_struct *tsk = data;

	struct ksig_info *nfo;
	struct ksig_handler *hdl;


	while (1) {

		if (list_empty(&tsk->ksig_queue)) {
			syscall_schedule();
			continue;
		}

		arch_local_irq_disable();
		ksig_lock();
		nfo = list_first_entry(&tsk->ksig_queue, struct ksig_info, node);
		list_del(&nfo->node);
		ksig_unlock();
		arch_local_irq_enable();

		list_for_each_entry(hdl, &tsk->ksig_handlers, node) {
			if (hdl->signal == nfo->signal) {

				if (!hdl->action.sa_sigaction)
					break;

				hdl->action.sa_sigaction(nfo->signal, &nfo->info, NULL);

				ksig_dec(1);

				kfree(nfo);

				ksig_tsk_sig_cnt_dec(tsk);

				arch_local_irq_disable();
				ksig_lock();
				tsk->sig_cnt--;
				ksig_unlock();
				arch_local_irq_enable();

				break;
			}
		}



	}
	return 0;
}


static int ksignal_add_task_node(struct ksig_handler *hdl)
{
	struct ksig_reg *ktmp;
	struct ksig_reg *ksig = NULL;


	list_for_each_entry(ktmp, &ksignals, node) {

		if (ktmp->signal == hdl->signal) {
			ksig = ktmp;
			break;
		}
	}

	if (ksig)
		goto success;


	ksig = kzalloc(sizeof(*ksig));
	if (!ksig)
		goto error;

	ksig->signal = hdl->signal;
	INIT_LIST_HEAD(&ksig->tasks);
	list_add_tail(&ksig->node, &ksignals);

success:
	list_add_tail(&hdl->task_node, &ksig->tasks);
	list_add_tail(&hdl->node, &hdl->tsk->ksig_handlers);

	return 0;

error:
	return -ENOMEM;
}


/**
 * @brief register a signal handler
 *
 * NOTE: this should not require locking, since a handler can only
 * be registered from a running task, so will not be free'd while
 * the call is going on
 */

int ksigaction(int signal, const struct ksig_action *act, struct ksig_action *oact)
{
	struct task_struct *tsk;
	struct ksig_handler *tmp;
	struct ksig_handler *hdl = NULL;



       	tsk = current_set[smp_cpu_id()]->task;

	if (!act && !oact)
		return -EINVAL;	/* you're not making sense */


	list_for_each_entry(tmp, &tsk->ksig_handlers, node) {
		if (tmp->signal == signal) {
			hdl = tmp;
			break;
		}
	}

	if (!act && !hdl)
		return -ENOENT;

	if (oact && hdl) /* requested signal config */
		memcpy(oact, &hdl->action, sizeof(*oact));

	/* XXX not usig sa_handler yet... */
	if (act->sa_sigaction == SIG_DFL) {
		if (hdl) {
			list_del(&hdl->node);
			kfree(hdl);
		}
		return 0;
	}

	if (!hdl) {
		hdl = kzalloc(sizeof(*hdl));
		if (!hdl)
			return -ENOMEM;

		hdl->signal = signal;
	}

	memcpy(&hdl->action, act, sizeof(*act));

	if (!tsk->sig)
		kthread_sigstack_create(tsk, ksig_exec);

	hdl->tsk = tsk;

	return ksignal_add_task_node(hdl);
}


/**
 * @brief queue a signal emission; will have no effect if a signal
 *	  is not registered to the particular task
 *
 * @note any signal passed to this function is currently treated as a broadcast
 */

int ksignal_send_info(int signal, siginfo_t *info)
{
	struct ksig_reg *ktmp;
	struct ksig_reg *ksig = NULL;

	struct ksig_handler *hdl;
	struct ksig_handler *htmp;

	struct ksig_info *nfo;


	list_for_each_entry(ktmp, &ksignals, node) {

		if (ktmp->signal == signal) {
			ksig = ktmp;
			break;
		}
	}

	if (!ksig) {
		pr_info(MSG "no registered tasks for signal %d\n", signal);
		return -ENOENT;
	}


	list_for_each_entry_safe(hdl, htmp, &ksig->tasks, task_node) {

		nfo = kzalloc(sizeof(*nfo));
		if (!nfo)
			return -ENOMEM;

		nfo->signal = signal;
		if (info)
			memcpy(&nfo->info, info, sizeof(*info));

		arch_local_irq_disable();
		ksig_lock();
		list_add_tail(&nfo->node, &hdl->tsk->ksig_queue);
		hdl->tsk->sig_cnt++;
		ksig_inc();
		ksig_unlock();
		arch_local_irq_enable();
	}


	return 0;
}


/*
 * NOTE: the individual schedulers call kthread_free() with scheduling locked,
 * which in turn calls this function, so we should never encounter an issue
 * with someone calling ksignal_send_info() above
 */

void ksignal_drop_task(struct task_struct *task)
{
	struct ksig_info *nfo;
	struct ksig_info *ntmp;

	struct ksig_handler *hdl;
	struct ksig_handler *htmp;



	list_for_each_entry_safe(nfo, ntmp, &task->ksig_queue, node) {
		arch_local_irq_disable();
		ksig_lock();
		list_del(&nfo->node);

		ksig_unlock();
		arch_local_irq_enable();
		kfree(nfo);
	}

	list_for_each_entry_safe(hdl, htmp, &task->ksig_handlers, node) {

		arch_local_irq_disable();
		ksig_lock();

		ksig_dec(hdl->tsk->sig_cnt);
		list_del(&hdl->node);
		list_del(&hdl->task_node);

		ksig_unlock();
		arch_local_irq_enable();

		kfree(hdl);
	}

	if (task->sig)
		kfree(task->sig->stack);
}


/**
 * @brief check if there are pending signals
 */

int ksignal_raised(void)
{
	return (ioread32be(&ksig_cnt) > 0);
}
