#ifndef _ASM_GENERIC_UNISTD_H
#define _ASM_GENERIC_UNISTD_H

/**
 * we hardcode all syscall numbers here, we won't have that many
 */

/* reserve a suitable slots in the syscall vector */
#define __NR_syscalls		16


#define __NR_read		0
#define __NR_write		1
#define __NR_alloc		2
#define __NR_free		3
#define __NR_gettime		4
#define __NR_nanosleep		5
#define __NR_grspw2		6
#define __NR_thread_create	7
#define __NR_sched_yield	8
#define __NR_watchdog		9
#define __NR_sched_prog_seg	10

#endif /* _ASM_GENERIC_UNISTD_H */
