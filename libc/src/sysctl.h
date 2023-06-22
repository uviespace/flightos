#ifndef SYSTCTL_H
#define SYSTCTL_H

#include <stddef.h>

int sysctl_show_attr(const char *path, const char *name, char *buf);
int sysctl_store_attr(const char *path, const char *name, const char *buf, size_t len);

#endif /* SYSTCTL_H */
