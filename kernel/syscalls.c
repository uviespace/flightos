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
#include <kernel/kthread.h>
#include <kernel/sched.h>
#include <kernel/sysctl.h>
#include <kernel/watchdog.h>
#include <kernel/application.h>


#undef __SYSCALL
#define __SYSCALL(nr, sym)	[nr] = sym,



#define stdin	0
#define stdout	1
#define stderr	2


/* XXX the user thread stuff must move somewhere else ... */
struct sys_sched_attr {

	/* period based scheduling for EDF, RMS, ... */
	unsigned long		period;           /* wakeup period */
	unsigned long		wcet;             /* max runtime per period*/
	unsigned long		deadline_rel;     /* time to deadline from begin of wakeup */

	/* static priority scheduling for RR, FIFO, ... */
	unsigned long		priority;

	int			on_cpu;						/* cpu number or THREAD_CPU_AFFINITY_NONE */
	enum sched_policy	policy;
    };

typedef struct {
	size_t   size;
	void	*th;				/* reference to the kernel thread object */
	int	(*thread_fn)(void *data);
	void	*data;
	char	*name;
	struct sys_sched_attr attr;
} thread_t;







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

/* XXX move all of this to the driver module, fix core enumeration */
extern struct spw_user_cfg spw_cfg[2];
SYSCALL_DEFINE1(grspw2, struct grspw2_data *, spw)
{
	if (!spw)
		return -EINVAL;

	if (spw->link > 2)
		return -EINVAL;

	switch (spw->op) {

	case GRSPW2_OP_ADD_PKT:
		 return grspw2_add_pkt(&spw_cfg[spw->link].spw, spw->hdr,  spw->hdr_size,
				       spw->data, spw->data_size);
	case GRSPW2_OP_ADD_RMAP:
		 return grspw2_add_rmap(&spw_cfg[spw->link].spw, spw->hdr,  spw->hdr_size,
					spw->non_crc_bytes, spw->data,
					spw->data_size);
	case GRSPW2_OP_GET_NUM_PKT_AVAIL:
			return grspw2_get_num_pkts_avail(&spw_cfg[spw->link].spw);
	case GRSPW2_OP_GET_NEXT_PKT_SIZE:
			return grspw2_get_next_pkt_size(&spw_cfg[spw->link].spw);
	case GRSPW2_OP_DROP_PKT:
			return grspw2_drop_pkt(&spw_cfg[spw->link].spw);
	case GRSPW2_OP_GET_PKT:
			return grspw2_get_pkt(&spw_cfg[spw->link].spw, spw->pkt);
	case GRSPW2_OP_GET_NEXT_PKT_EEP:
			return grspw2_get_next_pkt_eep(&spw_cfg[spw->link].spw);
	default:
			printk("SPW ERROR: unknown OP %d\n", spw->op);
		return -EINVAL;
	};


	return -EINVAL;
}


SYSCALL_DEFINE1(thread_create, thread_t *, t)
{
	char *c;
	struct task_struct *tsk;

	if (!t)
		return -EINVAL;

	if (t->size != sizeof(thread_t))
		return -EINVAL;


	/* overwrite any format specifiers, in case the user wants to be funny */
	c = t->name;
	while(c) {
		if ((*c) == '%')
			(*c) = 'x';
		c++;
	}


	tsk = kthread_create(t->thread_fn, t->data, t->attr.on_cpu, t->name);
	if (IS_ERR(tsk))
		return PTR_ERR(tsk);

	switch (t->attr.policy) {
	case SCHED_EDF:
		kthread_set_sched_edf(tsk, t->attr.period, t->attr.deadline_rel, t->attr.wcet);
		break;
	case SCHED_RR:
	case SCHED_OTHER:
	default:
		kthread_set_sched_rr(tsk, t->attr.priority);
		break;

	}

	/**
	 * XXX exposing this isn't the best idea, but we'll do it like this for now,
	 * just in case we need it.
	 * maybe we use some kind of lookup table rather than a memory address
	 */
	t->th = (void *) tsk;

	return kthread_wake_up(tsk);
}


SYSCALL_DEFINE0(sched_yield)
{
	sched_yield();

	return 0;
}


/* XXX move */
/**
 * if timeout == 0, enable is considered, otherwise ignored */
SYSCALL_DEFINE2(watchdog, unsigned long, timeout_ns, int, enable)
{
	if (timeout_ns)
		return watchdog_feed(timeout_ns);

	if (enable)
		return watchdog_set_mode(WATCHDOG_UNLEASH);

	return watchdog_set_mode(WATCHDOG_LEASH);
}


SYSCALL_DEFINE3(sched_prog_seg, void *, addr, int, argc, char **, argv)
{
	char *name = NULL;

	if (!addr)
		return -EINVAL;

	if (argv)
		name = argv[0];

	return application_load((void *) addr, name, KTHREAD_CPU_AFFINITY_NONE,
				argc, argv);
}


SYSCALL_DEFINE3(sysctl_show_attr, const char *, path, const char *, name, char *, buf)
{
	struct sysobj *obj;


	obj = sysset_find_obj(NULL, path);
	if (!obj)
		return -ENOENT;

	sysobj_show_attr(obj, name, buf);

	return 0;
}


SYSCALL_DEFINE4(sysctl_store_attr, const char *, path, const char *, name, const char *, buf, size_t, len)
{
	struct sysobj *obj;


	obj = sysset_find_obj(NULL, path);
	if (!obj)
		return -ENOENT;

	sysobj_store_attr(obj, name, buf, len);

	return 0;
}

#include <kernel/syscall.h>
__attribute__((noinline))
int syscall_sched_yield(void)
{
	return SYSCALL0(__NR_sched_yield);
}



/*
 * The sys_call_table array must be 4K aligned to be accessible from
 * kernel/entry.S.
 * XXX TODO make sure the etrap.S define for NR_syscalls matches
 */
void *syscall_tbl[__NR_syscalls] __aligned(4096) = {
	[0 ... __NR_syscalls - 1] = sys_ni_syscall,
	__SYSCALL(__NR_read,			sys_read)
	__SYSCALL(__NR_write,			sys_write)
	__SYSCALL(__NR_alloc,			sys_alloc)
	__SYSCALL(__NR_free,			sys_free)
	__SYSCALL(__NR_gettime,			sys_gettime)
	__SYSCALL(__NR_nanosleep,		sys_nanosleep)
	__SYSCALL(__NR_grspw2,			sys_grspw2)
	__SYSCALL(__NR_thread_create,		sys_thread_create)
	__SYSCALL(__NR_sched_yield,		sys_sched_yield)
	__SYSCALL(__NR_watchdog,		sys_watchdog)
	__SYSCALL(__NR_sched_prog_seg,		sys_sched_prog_seg)
	__SYSCALL(__NR_sysctl_show_attr,	sys_sysctl_show_attr)
	__SYSCALL(__NR_sysctl_store_attr,	sys_sysctl_store_attr)
};

