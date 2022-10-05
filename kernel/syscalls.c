/**
 * @file kernel/syscalls.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 *
 * @ingroup syscall
 *
 */


#include <kernel/time.h>
#include <kernel/err.h>
#include <kernel/kmem.h>
#include <kernel/string.h>
#include <kernel/tty.h>

#include <kernel/syscalls.h>
#include <asm-generic/unistd.h>

#include <grspw2.h>


#undef __SYSCALL
#define __SYSCALL(nr, sym)	[nr] = sym,



#define stdin	0
#define stdout	1
#define stderr	2



SYSCALL_DEFINE3(read, int, fd, void *, buf, size_t, count)
{
	printk("SYSCALL READ not implemented\n");
	return 0;
}

#if 1
SYSCALL_DEFINE3(write, int, fd, void *, buf, size_t, count)
{

	if (fd == stdout || fd == stderr)
		return tty_write(buf, count);


	printk("SYSCALL WRITE to fd not stdout or stderr not implemented\n");
	return 0;
}
#else

SYSCALL_DEFINE3(write, int, fd, void *, s, size_t, count)
{
	printk("SYSCALL WRITE int %d, string , other string %d\n", fd, s, count);
	return 1;
}
#endif

long sys_ni_syscall(int a, char *s)
{
	printk("SYSCALL %d not implemented\n");
	return 0;
}


SYSCALL_DEFINE2(alloc, size_t, size, void **, p)
{
	(*p) = kmalloc(size);

	if (!(*p))
		return -ENOMEM;

	return 0;
}


SYSCALL_DEFINE1(free, void *, p)
{
	kfree(p);

	return 0;
}

SYSCALL_DEFINE1(gettime, struct timespec *, t)
{
	struct timespec kt;


	if (!t)
		return -EINVAL;

	kt = get_ktime();

	memcpy(t, &kt, sizeof(struct timespec));

	return 0;
}


SYSCALL_DEFINE2(nanosleep, int, flags, struct timespec *, t)
{
#define TIMER_ABSTIME   4
	struct timespec ts;
	ktime wake;

	if (!t)
		return -EINVAL;

	memcpy(&ts, t, sizeof(struct timespec));

	if (flags & TIMER_ABSTIME)
		wake = timespec_to_ktime(ts);
	else
		wake = ktime_add(ktime_get(), timespec_to_ktime(ts));

#if 0
	printk("sleep for %g ms\n", 0.001 * (double)  ktime_ms_delta(wake, ktime_get()));
#endif
	/* just busy-wait for now */
	while (ktime_after(wake, ktime_get()));

	return 0;
}

/* XXX move all of this to the driver module */
extern struct spw_user_cfg spw_cfg;
SYSCALL_DEFINE1(grspw2, struct grspw2_data *, spw)
{
	if (!spw)
		return -EINVAL;

	switch (spw->op) {

	case GRSPW2_OP_ADD_PKT:
		 return grspw2_add_pkt(&spw_cfg.spw, spw->hdr,  spw->hdr_size,
				       spw->non_crc_bytes, spw->data,
				       spw->data_size);
	case GRSPW2_OP_GET_NUM_PKT_AVAIL:
			return grspw2_get_num_pkts_avail(&spw_cfg.spw);
	case GRSPW2_OP_GET_NEXT_PKT_SIZE:
			return grspw2_get_next_pkt_size(&spw_cfg.spw);
	case GRSPW2_OP_DROP_PKT:
			return grspw2_drop_pkt(&spw_cfg.spw);
	case GRSPW2_OP_GET_PKT:
			return grspw2_get_pkt(&spw_cfg.spw, spw->pkt);
	default:
			printk("SPW ERROR: unknown OP %d\n", spw->op);
		return -EINVAL;
	};


	return -EINVAL;
}

/*
 * The sys_call_table array must be 4K aligned to be accessible from
 * kernel/entry.S.
 */
void *syscall_tbl[__NR_syscalls] __aligned(4096) = {
	[0 ... __NR_syscalls - 1] = sys_ni_syscall,
	__SYSCALL(0,   sys_read)
	__SYSCALL(1,   sys_write)
	__SYSCALL(2,   sys_alloc)
	__SYSCALL(3,   sys_free)
	__SYSCALL(4,   sys_gettime)
	__SYSCALL(5,   sys_nanosleep)
	__SYSCALL(6,   sys_grspw2)
};

