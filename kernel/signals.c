#include <kernel/signals.h>
#include <errno.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/string.h>
#include <kernel/smp.h>

#define MSG "KSIGNAL: "


struct ksig_reg {
	int signal;
	struct list_head tasks;
	struct list_head node;
};

static LIST_HEAD(ksignals);


#include <kernel/syscall.h>
#include <asm/processor.h>
int syscall_sched_yield(void);
static int ksig_exec(void *data)
{
	struct task_struct *tsk = data;

	struct ksig_info *nfo;
	struct ksig_handler *hdl;


	while (1) {
		if (!tsk->signal || list_empty(&tsk->ksig_queue)) {
			/* switch back; atomicity should not be a problem here,
			 * as worst-case, we will land back here...I hope...
			 */
			tsk->active = &tsk->tsk;
			syscall_sched_yield();
			continue;
		}

		nfo = list_first_entry(&tsk->ksig_queue, struct ksig_info, node);
		list_del(&nfo->node);

		list_for_each_entry(hdl, &tsk->ksig_handlers, node) {
			if (hdl->signal == nfo->signal) {
				if (!hdl->action.sa_sigaction)
					break;

				hdl->action.sa_sigaction(nfo->signal, &nfo->info, NULL);

				kfree(nfo);
				break;
			}
		}

		tsk->signal--;
	}
	return 0;
}


static int ksignal_add_task_node(int signal,
				 struct ksig_handler *hdl,
				 struct task_struct *tsk)
{
	struct ksig_reg *ktmp;
	struct ksig_reg *ksig = NULL;


	list_for_each_entry(ktmp, &ksignals, node) {

		if (ktmp->signal == signal) {
			ksig = ktmp;
			break;
		}
	}

	if (ksig)
		goto success;


	ksig = kmalloc(sizeof(*ksig));
	if (!ksig)
		goto error;

	ksig->signal = signal;
	INIT_LIST_HEAD(&ksig->node);
	INIT_LIST_HEAD(&ksig->tasks);
	list_move_tail(&ksig->node, &ksignals);

success:
	list_move_tail(&tsk->ksig_node, &ksig->tasks);
	list_move_tail(&hdl->node, &tsk->ksig_handlers);

	return 0;

error:
	list_del(&hdl->node);
	list_del(&tsk->ksig_node);

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
		INIT_LIST_HEAD(&hdl->node);
	}

	memcpy(&hdl->action, act, sizeof(*act));

	if (!tsk->sig)
		kthread_sigstack_create(tsk, ksig_exec);

	return ksignal_add_task_node(signal, hdl, tsk);
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

	struct task_struct *tsk;
	struct task_struct *ttmp;

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

	list_for_each_entry_safe(tsk, ttmp, &ksig->tasks, ksig_node) {

		nfo = kmalloc(sizeof(*nfo));
		if (!nfo)
			return -ENOMEM;

		INIT_LIST_HEAD(&nfo->node);
		nfo->signal = signal;
		if (info)
			memcpy(&nfo->info, info, sizeof(*info));

		list_add_tail(&nfo->node, &tsk->ksig_queue);
		tsk->signal++;

		schedule();
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



	list_del(&task->ksig_node);

	list_for_each_entry_safe(nfo, ntmp, &task->ksig_queue, node) {
		list_del(&nfo->node);
		kfree(nfo);
	}

	list_for_each_entry_safe(hdl, htmp, &task->ksig_handlers, node) {
		list_del(&hdl->node);
		kfree(hdl);
	}

	kfree(task->sig->stack);
}
