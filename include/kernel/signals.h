/**
 * @file include/kernel/signals.h
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

/**
 * @brief signal info structure compatible with P1003.1b-1993
 */
typedef struct {
        int si_signo;		/*!< signal number */
	int si_code;		/*!< signal code */
        union sigval si_value;	/*!< signal value */
} siginfo_t;

/**
 * @brief kernel signal action structure
 */
struct ksig_action {
	void     (*sa_handler)(int);		/*!< simple signal handler */
	void     (*sa_sigaction)(int, siginfo_t *, void *); /*!< detailed signal handler */
#if 0
	sigset_t   sa_mask;
#endif
	int        sa_flags;			/*!< signal action flags (e.g., SA_SIGINFO) */
#if 0
	void     (*sa_restorer)(void);
#endif
};

/**
 * @brief internal signal handler registration entry
 */
struct ksig_handler {
	int signal;				/*!< signal number */
	struct task_struct *tsk;		/*!< task registered for this signal */
	struct ksig_action action;		/*!< signal action configuration */
	struct list_head node;			/*!< node in global handler list */
	struct list_head task_node;		/*!< node in per-task handler list */
};

/**
 * @brief queued signal information
 */
struct ksig_info {
	int signal;				/*!< signal number */
	siginfo_t info;				/*!< signal info payload */
	struct list_head node;			/*!< node in signal queue */
};


/**
 * @brief send a signal with info to all registered handlers
 * @param signal: signal number to send
 * @param info: pointer to signal info (may be NULL for simple signal)
 * @return 0 on success, negative error code on failure
 */
int ksignal_send_info(int signal, siginfo_t *info);

/**
 * @brief set or get signal actions for a signal number
 * @param signal: signal number
 * @param act: new signal action (or NULL to query only)
 * @param oact: buffer to store the old signal action (or NULL)
 * @return 0 on success, negative error code on failure
 */
int ksigaction(int signal, const struct ksig_action *act, struct ksig_action *oact);

/**
 * @brief remove all signal registrations for a task
 * @param task: the task whose signals should be dropped
 */
void ksignal_drop_task(struct task_struct *task);

/**
 * @brief check if there are any pending signals
 * @return non-zero if signals are pending, 0 otherwise
 */
int ksignal_raised(void);

#endif /* _KERNEL_SIGNALS_H_ */
