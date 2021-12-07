#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <xen_printf.h>


/**
 * we allow syscalls with at most 6 arguments
 * since we don't want to rely on the existence of memcpy()
 * and/or stdarg, we just define 7 macros for syscall0-syscall6
 *
 * each argument is cast to long, inserted to the %o registers
 * as the argument vector %i in the next frame
 *
 * the syscall ID is in %g1, so we can access it immediately after taking
 * the trap.
 */


#define SYSCALL6(id, a0, a1, a2, a3, a4, a5)			\
({								\
	long _a0 = (long) (a0);					\
	long _a1 = (long) (a1);					\
	long _a2 = (long) (a2);					\
	long _a3 = (long) (a3);					\
	long _a4 = (long) (a4);					\
	long _a5 = (long) (a5);					\
	register long _g1 __asm__ ("g1") = (id);		\
	register long _o0 __asm__ ("o0") = _a0;			\
	register long _o1 __asm__ ("o1") = _a1;			\
	register long _o2 __asm__ ("o2") = _a2;			\
	register long _o3 __asm__ ("o3") = _a3;			\
	register long _o4 __asm__ ("o4") = _a4;			\
	register long _o5 __asm__ ("o5") = _a5;			\
								\
	__asm__ __volatile__(					\
			     "mov       %1, %%g1    \n\t"	\
			     "mov       %2, %%o0    \n\t"	\
			     "mov       %3, %%o1    \n\t"	\
			     "mov       %4, %%o2    \n\t"	\
			     "mov       %5, %%o3    \n\t"	\
			     "mov       %6, %%o4    \n\t"	\
			     "mov       %7, %%o5    \n\t"	\
			     "ta	0x0	    \n\t"	\
			     "mov	%%o0, %0    \n\t"	\
			     : "=r" (_o0)			\
			     : "r"  (_g1),			\
			       "r"  (_o0), "r"  (_o1),		\
			       "r"  (_o2), "r"  (_o3),		\
			       "r"  (_o4), "r"  (_o5)		\
			     : "memory");			\
	_o0;	/* retval */					\
})

#define SYSCALL5(id, a0, a1, a2, a3, a4)			\
({								\
	long _a0 = (long) (a0);					\
	long _a1 = (long) (a1);					\
	long _a2 = (long) (a2);					\
	long _a3 = (long) (a3);					\
	long _a4 = (long) (a4);					\
	register long _g1 __asm__ ("g1") = (id);		\
	register long _o0 __asm__ ("o0") = _a0;			\
	register long _o1 __asm__ ("o1") = _a1;			\
	register long _o2 __asm__ ("o2") = _a2;			\
	register long _o3 __asm__ ("o3") = _a3;			\
	register long _o4 __asm__ ("o4") = _a4;			\
								\
	__asm__ __volatile__(					\
			     "mov       %1, %%g1    \n\t"	\
			     "mov       %2, %%o0    \n\t"	\
			     "mov       %3, %%o1    \n\t"	\
			     "mov       %4, %%o2    \n\t"	\
			     "mov       %5, %%o3    \n\t"	\
			     "mov       %6, %%o4    \n\t"	\
			     "ta	0x0	    \n\t"	\
			     "mov	%%o0, %0    \n\t"	\
			     : "=r" (_o0)			\
			     : "r"  (_g1),			\
			       "r"  (_o0), "r"  (_o1),		\
			       "r"  (_o2), "r"  (_o3),		\
			       "r"  (_o4)			\
			     : "memory");			\
	_o0;	/* retval */					\
})

#define SYSCALL4(id, a0, a1, a2, a3)				\
({								\
	long _a0 = (long) (a0);					\
	long _a1 = (long) (a1);					\
	long _a2 = (long) (a2);					\
	long _a3 = (long) (a3);					\
	register long _g1 __asm__ ("g1") = (id);		\
	register long _o0 __asm__ ("o0") = _a0;			\
	register long _o1 __asm__ ("o1") = _a1;			\
	register long _o2 __asm__ ("o2") = _a2;			\
	register long _o3 __asm__ ("o3") = _a3;			\
								\
	__asm__ __volatile__(					\
			     "mov       %1, %%g1    \n\t"	\
			     "mov       %2, %%o0    \n\t"	\
			     "mov       %3, %%o1    \n\t"	\
			     "mov       %4, %%o2    \n\t"	\
			     "mov       %5, %%o3    \n\t"	\
			     "ta	0x0	    \n\t"	\
			     "mov	%%o0, %0    \n\t"	\
			     : "=r" (_o0)			\
			     : "r"  (_g1),			\
			       "r"  (_o0), "r"  (_o1),		\
			       "r"  (_o2), "r"  (_o3)		\
			     : "memory");			\
	_o0;	/* retval */					\
})

#define SYSCALL3(id, a0, a1, a2)				\
({								\
	long _a0 = (long) (a0);					\
	long _a1 = (long) (a1);					\
	long _a2 = (long) (a2);					\
	register long _g1 __asm__ ("g1") = (id);		\
	register long _o0 __asm__ ("o0") = _a0;			\
	register long _o1 __asm__ ("o1") = _a1;			\
	register long _o2 __asm__ ("o2") = _a2;			\
								\
	__asm__ __volatile__(					\
			     "mov       %1, %%g1    \n\t"	\
			     "mov       %2, %%o0    \n\t"	\
			     "mov       %3, %%o1    \n\t"	\
			     "mov       %4, %%o2    \n\t"	\
			     "ta	0x0	    \n\t"	\
			     "mov	%%o0, %0    \n\t"	\
			     : "=r" (_o0)			\
			     : "r"  (_g1),			\
			       "r"  (_o0), "r"  (_o1),		\
			       "r"  (_o2)			\
			     : "memory");			\
	_o0;	/* retval */					\
})

#define SYSCALL2(id, a0, a1)					\
({								\
	long _a0 = (long) (a0);					\
	long _a1 = (long) (a1);					\
	register long _g1 __asm__ ("g1") = (id);		\
	register long _o0 __asm__ ("o0") = _a0;			\
	register long _o1 __asm__ ("o1") = _a1;			\
								\
	__asm__ __volatile__(					\
			     "mov       %1, %%g1    \n\t"	\
			     "mov       %2, %%o0    \n\t"	\
			     "mov       %3, %%o1    \n\t"	\
			     "ta	0x0	    \n\t"	\
			     "mov	%%o0, %0    \n\t"	\
			     : "=&r" (_o0)			\
			     : "r"  (_g1),			\
			       "r"  (_o0), "r"  (_o1)		\
			     : "memory");			\
	_o0;	/* retval */					\
})

#define SYSCALL1(id, a0)					\
({								\
	long _a0 = (long) (a0);					\
	register long _g1 __asm__ ("g1") = (id);		\
	register long _o0 __asm__ ("o0") = _a0;			\
								\
	__asm__ __volatile__(					\
			     "mov       %1, %%g1    \n\t"	\
			     "mov       %2, %%o0    \n\t"	\
			     "ta	0x0	    \n\t"	\
			     "mov	%%o0, %0    \n\t"	\
			     : "=&r" (_o0)			\
			     : "r"  (_g1),			\
			       "r"  (_o0)			\
			     : "memory");			\
	_o0;	/* retval */					\
})

#define SYSCALL0(id)						\
({								\
	long _a0 = (long) (a0);					\
	register long _g1 __asm__ ("g1") = (id);		\
								\
	__asm__ __volatile__(					\
			     "mov       %1, %%g1    \n\t"	\
			     "ta	0x0	    \n\t"	\
			     "mov	%%o0, %0    \n\t"	\
			     : "=&r" (_o0)			\
			     : "r"  (_g1),			\
			       "r"  (_o0)			\
			     : "memory");			\
	_o0;	/* retval */					\
})







int main(void)
{
	int ret;

	char hi[] = "Hello World, i am an executable";
	char hi2[] = "And I am a string!";

	ret = SYSCALL2(0, __LINE__, hi);
	x_printf("syscall returned = %d\n", ret);

	ret = SYSCALL3(1, __LINE__, hi, hi2);
	x_printf("syscall returned = %d\n", ret);


	x_printf("entering infinite loop\n");

	while (1);

	return 0;
}
