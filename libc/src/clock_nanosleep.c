#include <sys/types.h>
#include <syscall.h>


static int sys_nanosleep(int flags, const struct timespec *rqtp)
{
	return SYSCALL2(5, flags, rqtp);
}


/**
 * @note we ignore all arguments except rqtp and flags
 */

int clock_nanosleep(__attribute__((unused)) clockid_t clock_id,
		    int flags,
		    const struct timespec *rqtp,
		    __attribute__((unused)) struct timespec *rmtp)
{
	return sys_nanosleep(flags, rqtp);
}
