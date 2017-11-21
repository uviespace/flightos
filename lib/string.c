/*+
 * @file lib/string.c
 *
 * @ingroup string
 *
 * @defgroup string String Manipulation
 *
 * @brief implememnts generic string manipulation functions
 *
 * @note some of theses are just wrappers
 * @todo these functions are very generic, for performance, add architecture
 * specific implementations
 */


#include <kernel/kmem.h>
#include <kernel/export.h>
#include <kernel/types.h>
#include <kernel/string.h>


/**
 * @brief compare two strings s1 and s2
 *
 * @returns <0, 0 or > 0 if s1 is less than, matches or greater than s2
 */

#include <kernel/printk.h>
int strcmp(const char *s1, const char *s2)
{
	unsigned char c1, c2;

	do {
		c1 = (*(s1++));
		c2 = (*(s2++));

		if (c1 != c2) {
			if (c1 < c2)
				return -1;
			else
				return  1;
		}

	} while (c1);

	return 0;
}
EXPORT_SYMBOL(strcmp);


/**
 * @brief compare two strings s1 and s2 by comparing at most n bytes
 *
 * @returns <0, 0 or > 0 if s1 is less than, matches or greater than s2
 */
int strncmp(const char *s1, const char *s2, size_t n)
{
	unsigned char c1, c2;

	while (n) {

		c1 = (*(s1++));
		c2 = (*(s2++));

		if (c1 != c2) {
			if (c1 < c2)
				return -1;
			else
				return 1;
		}

		if (!c1)
			break;

		n--;
	}

	return 0;
}
EXPORT_SYMBOL(strncmp);


/**
 * @brief locate first occurrence of a set of characters in string accept
 *
 * @param s	 the string to be searched
 * @param accept the characters to search for
 *
 * @returns a pointer to the character in s that matches one of the characters
 *	    in accept
 */

char *strpbrk(const char *s, const char *accept)
{
	const char *c1, *c2;


	for (c1 = s; *c1 != '\0'; c1++)
		for (c2 = accept; (*c2) != '\0'; c2++)
			if ((*c1) == (*c2))
				return (char *) c1;

	return NULL;
}
EXPORT_SYMBOL(strpbrk);



/**
 * @brief split a string into tokens
 *
 * @param stringp the string to be searched
 * @param delim   the characters to search for
 *
 * @note stringp is updated to point past the token
 *
 * @returns the original location of stringp
 */

char *strsep(char **stringp, const char *delim)
{
	char *start;
	char *end;


	start = *stringp;

	if (!start)
		return NULL;

	end = strpbrk(start, delim);

	if (end) {
		(*end) = '\0';
		end++;
	}

	*stringp = end;

	return start;
}
EXPORT_SYMBOL(strsep);


/**
 * @brief return a pointer to a new string which is a duplicate of string s
 *
 * @parm s the string to duplicate
 *
 * @note the pointer is allocated using kmalloc() and may be freed with kfree()
 */

char *strdup(const char *s)
{
        size_t len;

        char *dup;

        if (!s)
                return NULL;

        len = strlen(s) + 1;
        dup = kzalloc(len);

        if (dup)
                memcpy(dup, s, len);

        return dup;
}
EXPORT_SYMBOL(strdup);


/**
 * @brief calculate the length of a string
 *
 * @param s the string
 *
 * @returns the number of characters in s
 */

size_t strlen(const char *s)
{
	const char *c;


	for (c = s; (*c) != '\0'; c++);

	return c - s;
}
EXPORT_SYMBOL(strlen);


/**
 * @brief finds the first occurrence of the substring needle in the string
 *        haystack
 *
 * @param haystack the string to be searched
 * @param needle   the string to search for
 *
 * @returns a pointer to the beginning of a substring or NULL if not found
 */

char *strstr(const char *haystack, const char *needle)
{
        size_t len_h;
        size_t len_n;


        len_n = strlen(needle);

        if (!len_n)
                return (char *) haystack;

        len_h = strlen(haystack);

        for ( ; len_h >= len_n; len_h--) {

                if (!memcmp(haystack, needle, len_n))
                        return (char *) haystack;

                haystack++;
        }

        return NULL;
}
EXPORT_SYMBOL(strstr);

/**
 * @brief compares the first n bytes of the memory areas s1 and s2
 *
 * @param s1 the first string
 * @param s2 the second string
 * @param n the number of bytes to compare
 *
 * @returns <0, 0 or > 0 if s1 is the first n bytes are found of s1 are found to
 *          be less than, to match or be greater than s2
 *
 * @note s1 and s2 are interpreted as unsigned char
 */

int memcmp(const void *s1, const void *s2, size_t n)
{
        const unsigned char *su1, *su2;


	su1 = (const unsigned char *) s1;
	su2 = (const unsigned char *) s2;

	while (n) {

		if ((*su1) != (*su2)) {
			if ((*su1) < (*su2))
				return -1;
			else
				return  1;
		}

		su1++;
		su2++;
		n--;
	}

	return 0;
}
EXPORT_SYMBOL(memcmp);


/**
 * @brief copy a memory area
 *
 * @param dest the destination memory area
 * @param src the source memory area
 * @param n the number of bytes to copy
 *
 * @returns a pointer to dest
 */

void *memcpy(void *dest, const void *src, size_t n)
{
	char *d;

	const char *s;


	d = dest;
	s = src;

	while (n--) {
		(*d) = (*s);
		d++;
		s++;
	}

	return dest;
}

EXPORT_SYMBOL(memcpy);


/**
 * @brief copy a '\0' terminated string
 *
 * @param dest the destination of the string
 * @param src the source of string
 *
 * @return a pointer to dest
 *
 * @note strings may not overlap
 *
 */

char *strcpy(char *dest, const char *src)
{
	char *tmp;


	tmp = dest;

	while ((*dest++ = *src++) != '\0');

	return tmp;
}
EXPORT_SYMBOL(strcpy);



/**
 * @brief set the first n bytes of the area starting at s to zero (bytes
 *        containing '\0')
 *
 * @param s the start of the area
 *
 * @param n the number of bytes to set to zero
 *
 */

void bzero(void *s, size_t n)
{
	char *c;


	c = (char *) s;

	while (n--) {
		(*c) = '\0';
		c++;
	}
}
EXPORT_SYMBOL(bzero);



#include <stdarg.h>
#include <limits.h>

int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/**
 * @brief print a string into a buffer
 *
 * @param str    the destination buffer
 * @param format the format string buffer
 * @param ...    arguments to the format string
 *
 * @return the number of characters written to buf
 */

int sprintf(char *str, const char *format, ...)
{
	int n;
	va_list ap;


	va_start(ap, format);
	n = vsnprintf(str, INT_MAX, format, ap);
	va_end(ap);

	return n;
}
EXPORT_SYMBOL(sprintf);


/**
 * @brief check if a character is a white space
 *
 * @param c the character to test
 *
 * @returns 0 if not a space
 *
 * @note c must have the value of an unsigned char or EOF
 *
 */

int isspace(int c)
{
	if (c == ' ')
		return 1;

	if (c == '\t')
		return 1;

	if (c == '\n')
		return 1;

	if (c == '\v')
		return 1;

        if (c == '\f')
		return 1;

       	if (c == '\r')
		return 1;


	return 0;
}
EXPORT_SYMBOL(isspace);

/**
 * @brief convert a string to an integer
 *
 * @param nptr the string to convert
 *
 * @returns res the converted integer
 */

int atoi(const char *nptr)
{
	unsigned int d;

	int res = 0;
	int neg= 0;


	for (; isspace(*nptr); ++nptr);

	if (*nptr == '-') {
		nptr++;
		neg = 1;
	} else if (*nptr == '+') {
		nptr++;
	}

	while (1) {
		d = (*nptr) - '0';

		if (d > 9)
			break;

		res = res * 10 + (int) d;
		nptr++;
	}

	if (neg)
		res = -res;

	return res;
}
EXPORT_SYMBOL(atoi);
