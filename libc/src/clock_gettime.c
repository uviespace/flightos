#include <sys/types.h>
#include <syscall.h>


static int sys_clock_gettime(struct timespec *tp)
{
	return SYSCALL1(4, tp);
}


/**
 * @note at this point, we only get the kernel time (== system uptime)
 */

int clock_gettime(__attribute__((unused)) clockid_t clk_id, struct timespec *tp)
{
	return sys_clock_gettime(tp);
}
