#ifndef SIGNALS_H
#define SIGNALS_H

/* the signal.h in bcc2 stupidly requires __rtems__ to be
 * defined, otherwise it won't add an sa_sigaction to
 * struct sigaction; we can't do that, since that would
 * fuck up a lot of other stuff; I therefore prefix these;
 * we won't really need the signal system for PC compatibility
 * for now, but in case, we can just add an alias in our
 * application softwares
 */

union xsigval {
  int    sival_int;
  void  *sival_ptr;
};

/* we want to be compatible to P1003.1b-1993 signals */
typedef struct {
        int si_signo;
	int si_code;
        union xsigval si_value;
} xsiginfo_t;

struct xsigaction {
	void     (*sa_handler)(int);
	void     (*sa_sigaction)(int, xsiginfo_t *, void *);
	int        sa_flags;
};

int xsigaction(int sig, const struct xsigaction *act, struct xsigaction *oact);

#endif /* SIGNAL_H */
