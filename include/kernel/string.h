/**
 * @file include/kernel/string.h
 *
 * @ingroup string
 */

#ifndef _KERNEL_STRING_H_
#define _KERNEL_STRING_H_

#include <kernel/types.h>
#include <stdarg.h>
#include <limits.h>


int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strpbrk(const char *s, const char *accept);
char *strsep(char **stringp, const char *delim);
char *strdup(const char *s);

char *strchr(const char *s, int c);

char *strstr(const char *haystack, const char *needle);
size_t strlen(const char *s);
int memcmp(const void *s1, const void *s2, size_t n);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
char *strcpy(char *dest, const char *src);
void bzero(void *s, size_t n);

int isdigit(int c);
int isspace(int c);
int isalpha(int c);
int isupper(int c);
int islower(int c);

int atoi(const char *nptr);
long int strtol(const char *nptr, char **endptr, int base);


int vprintf(const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);


#endif /* _KERNEL_STRING_H_ */
