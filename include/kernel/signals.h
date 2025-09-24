/**
 * @file include/kernel/sched.h
 *
 * NOTE: this is a very rudimentary implementation of signals;
 * we currently only require them to emit driver-related info to running
 * applications that are critical with regard to system operations which cannot
 * rely on polling
 *
 * we also don't have an arch-specific implementation, as this one should do
 * for the time being
 *
 * note that all signals will be broadcast signals, a task will be added to
 * a list which holds a registered signal and the tasks registered to those
 * signals
 *
 *
 * XXX or a signal array which expands as needed base on the index of the
 * signals -> more sensible I think
 */

#ifndef _KERNEL_SIGNALS_H_
#define _KERNEL_SIGNALS_H_

#include <stdint.h>
#include <list.h>

#include <kernel/kthread.h>


#define SA_SIGINFO	0x1UL

/* XXX NOTE: we do not currently support masking of signals, since
 * all signals are related to propagation to userspace and must be explicitly
 * registered to
 */
#if 0
#define SIG_BLOCK	0x01
#define SIG_UNBLOCK	0x02

typedef sigset_t uint32_t;
#endif


#define SIG_DFL	NULL	/* XXX meh, but good enough for now */

union sigval {
  int    sival_int;
  void  *sival_ptr;
};

/* we want to be compatible to P1003.1b-1993 signals */
typedef struct {
        int si_signo;
	int si_code;
        union sigval si_value;
} siginfo_t;

/* XXX we just go with the void pointer for sa_sigaction instead of
 * the newer ucontext_t for now
 */

struct ksig_action {
	void     (*sa_handler)(int);
	void     (*sa_sigaction)(int, siginfo_t *, void *);
#if 0
	sigset_t   sa_mask;
#endif
	int        sa_flags;
#if 0
	void     (*sa_restorer)(void);
#endif
};

struct ksig_handler {
	int signal;
	struct task_struct *tsk;
	struct ksig_action action;
	struct list_head node;
	struct list_head task_node;
};

struct ksig_info {
	int signal;
	siginfo_t info;
	struct list_head node;
};


int ksignal_send_info(int signal, siginfo_t *info);

int ksigaction(int signal, const struct ksig_action *act, struct ksig_action *oact);

void ksignal_drop_task(struct task_struct *task);

#endif /* _KERNEL_SIGNALS_H_ */
