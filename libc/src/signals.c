#include <syscalls.h>

/* see header */
int xsigaction(int sig, const struct xsigaction *act, struct xsigaction *oact)
{
#if 0	/* if we only had a sensible sys/signal.h in bcc2 ... */
	int ret;

	struct ksig_action kact  = {0};
	struct ksig_action koact = {0};

	struct ksig_action *p_koact      = NULL;
	const struct ksig_action *p_kact = NULL;


	if (act) {
		kact.sa_handler   = act->sa_handler;
		kact.sa_sigaction = act->sa_sigaction;
		kact.sa_flags     = act->sa_flags;
		p_kact = &kact;
	}

	ret = sys_sigaction(sig, p_kact, p_koact);

	if (oact)
		p_koact = &koact;

	return ret;
#else
	return sys_sigaction(sig, act, oact);
#endif
}
