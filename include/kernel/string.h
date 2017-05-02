/**
 * @file include/kernel/string.h
 *
 * @ingroup string
 */

#ifndef _KERNEL_STRING_H_
#define _KERNEL_STRING_H_

#include <kernel/types.h>


int sprintf(char *str, const char *format, ...);
int strcmp(const char *s1, const char *s2);
char *strpbrk(const char *s, const char *accept);
char *strsep(char **stringp, const char *delim);
char *strdup(const char *s);

char *strstr(const char *haystack, const char *needle);
size_t strlen(const char *s);
int memcmp(const void *s1, const void *s2, size_t n);

void *memcpy(void *dest, const void *src, size_t n);
char *strcpy(char *dest, const char *src);
void bzero(void *s, size_t n);

int isspace(int c);
int atoi(const char *nptr);


#endif /* _KERNEL_STRING_H_ */
