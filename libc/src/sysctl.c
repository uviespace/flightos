#include <syscalls.h>
#include <sysctl.h>


int sysctl_show_attr(const char *path, const char *name, char *buf)
{
	return sys_sysctl_show_attr(path, name, buf);
}


int sysctl_store_attr(const char *path, const char *name, const char *buf, size_t len)
{
	return sys_sysctl_store_attr(path, name, buf, len);
}
