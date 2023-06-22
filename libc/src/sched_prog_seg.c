#include <syscalls.h>
#include <sched_prog_seg.h>


int sched_prog_seg(void *addr, int argc, char *argv[])
{
	return sys_sched_prog_seg(addr, argc, argv);
}
