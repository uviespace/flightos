#include <syscalls.h>
#include <watchdog.h>

int watchdog_enable(void)
{
	return (int) sys_watchdog(0, 1);
}

int watchdog_disable(void)
{
	return (int) sys_watchdog(0, 0);
}

int watchdog_feed(unsigned long timeout_ns)
{
	return (int) sys_watchdog(timeout_ns, 0);
}
