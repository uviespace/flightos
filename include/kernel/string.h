/**
 * @file include/kernel/string.h
 */

#ifndef _KERNEL_STRING_H_
#define _KERNEL_STRING_H_

int sprintf(char *str, const char *format, ...);
int strcmp(const char *s1, const char *s2);
char *strpbrk(const char *s, const char *accept);
char *strsep(char **stringp, const char *delim);
size_t strlen(const char *s);

void *memcpy(void *dest, const void *src, size_t n);

int isspace(int c);
int atoi(const char *nptr);


#endif /* _KERNEL_STRING_H_ */
