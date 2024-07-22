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
#include <kernel/printk.h>
#include <kernel/log2.h>
#include <kernel/bitops.h>
#include <kernel/kernel.h>
#include <kernel/tty.h>


/**
 * @brief compare two strings s1 and s2
 *
 * @returns <0, 0 or > 0 if s1 is less than, matches or greater than s2
 */

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
 * @brief get length of a prefix substring
 *
 * @param s       the string to be searched
 * @param accept  the string segment to search for
 */

size_t strspn(const char *s, const char *accept)
{
	const char *c;
	const char *a;

	size_t cnt = 0;


	for (c = s; (*c); c++) {

		for (a = accept; (*a); a++) {

			if ((*c) == (*a))
				break;
		}

		if (!(*a))
			break;

		cnt++;
	}

	return cnt;
}
EXPORT_SYMBOL(strspn);


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


char *strtok(char *s, const char *delim)
{
	static char *s_prev;

	char *token;


	if (!s)
		s = s_prev;


	s += strspn(s, delim);

	if (!(*s)) {
		s_prev = s;
		return NULL;
	}

	token = s;
	s = strpbrk(token, delim);

	if (s) {
		*s = '\0';
		s_prev = s + 1;
	} else {
		s_prev = memchr(token, '\0', -1);
	}

	return token;

}
EXPORT_SYMBOL(strtok);



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


__diag_push();
__diag_ignore(GCC, 7, "-Wnonnull-compare", "can't guarantee that this is nonnull");
        if (!s)
                return NULL;
__diag_pop();

        len = strlen(s) + 1;
        dup = kzalloc(len);

        if (dup)
                memcpy(dup, s, len);

        return dup;
}
EXPORT_SYMBOL(strdup);


/**
 * @brief locate a character in string
 *
 * @param s the string to search in
 * @param c the character to search for
 *
 * @returns a pointer to the first matched character or NULL if not found
 *
 * @note the terminating null byte is considered part of the string, so that if
 *	 c is given as '\0', the function returns a pointer to the terminator
 */

char *strchr(const char *s, int c)
{
	while ((*s) != (char) c) {

		if ((*s) == '\0')
			return NULL;

		s++;
	}

	return (char *) s;
}
EXPORT_SYMBOL(strchr);


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
 * @brief copy a memory area src that may overlap with area dest
 *
 * @param dest the destination memory area
 * @param src the source memory area
 * @param n the number of bytes to copy
 *
 * @returns a pointer to dest
 */

void *memmove(void *dest, const void *src, size_t n)
{
	char *d;

	const char *s;

	if (dest <= src) {

		d = dest;
		s = src;

		while (n--) {
			(*d) = (*s);
			d++;
			s++;
		}

	} else {

		d = dest;
		d += n;

		s = src;
		s += n;

		while (n--) {
			d--;
			s--;
			(*d) = (*s);
		}

	}
	return dest;
}
EXPORT_SYMBOL(memmove);


/**
 * @brief	scan memory for a character
 * @param s	the memory area
 * @param c	the byte to search
 * @param n	The size of the area
 *
 * @returns	the address of the first occurrence of c or NULL if not found
 */

void *memchr(const void *s, int c, size_t n)
{
        const unsigned char *p = s;


        while (n--) {

                if ((unsigned char)c == *p++)
                        return (void *)(p - 1);
        }

        return NULL;
}
EXPORT_SYMBOL(memchr);






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
	memset(s, '\0', n);
}
EXPORT_SYMBOL(bzero);


/**
 * @brief writes the string s and a trailing newline to stdout
 *
 * @param str    the destination buffer
 * @param format the format string buffer
 * @param ...    arguments to the format string
 *
 * @return the number of characters written to buf
 */

int puts(const char *s)
{
	int n;

__diag_push();
__diag_ignore(GCC, 7, "-Wformat-truncation", "a NULL destination pointer indended");

	n = vsnprintf(NULL, INT_MAX, s, NULL);
	n+= vsnprintf(NULL, INT_MAX, "\n", NULL);

__diag_pop();

	return n;
}
EXPORT_SYMBOL(puts);


/**
 * @brief  writes the character c, cast to an unsigned char, to stdout
 *
 * @param c the character to write
 *
 * @return the number of characters written to buf
 */

int putchar(int c)
{
	char o = 0xff & c;

	return tty_write(&o, 1);
}





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
 * @brief print a string into a buffer of a given maximum size
 *
 * @param str    the destination buffer
 * @param size	 the size of the destination buffer
 * @param format the format string buffer
 * @param ...    arguments to the format string
 *
 * @return the number of characters written to buf
 */

int snprintf(char *str, size_t size, const char *format, ...)
{
	int n;
	va_list ap;


	va_start(ap, format);
	n = vsnprintf(str, size, format, ap);
	va_end(ap);

	return n;
}
EXPORT_SYMBOL(snprintf);


/**
 * @brief format a string and print it into a buffer
 *
 * @param str    the destination buffer
 * @param format the format string buffer
 * @param ...    arguments to the format string
 *
 * @return the number of characters written to buf
  */

int vsprintf(char *str, const char *format, va_list ap)
{
	return vsnprintf(str, INT_MAX, format, ap);
}
EXPORT_SYMBOL(vsprintf);



/**
 * @brief format a string and print it to the standard output
 *
 * @param format the format string buffer
 * @param ...    arguments to the format string
 *
 * @return the number of characters written to stdout
  */

int vprintf(const char *format, va_list ap)
{
__diag_push();
__diag_ignore(GCC, 7, "-Wformat-truncation", "a NULL destination pointer indended");

	return vsnprintf(NULL, INT_MAX, format, ap);

__diag_pop();
}
EXPORT_SYMBOL(vprintf);


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
 * @brief check if a character is a digit
 *
 * @param c the character to test
 *
 * @returns 0 if not a digit
 */

int isdigit(int c)
{
        return '0' <= c && c <= '9';
}
EXPORT_SYMBOL(isdigit);


/**
 * @brief check if a character is from the alphabet
 *
 * @param c the character to test
 *
 * @returns 0 if not in the alphabet
 *
 * @note in ASCII, 'a-z' are 'A-Z' + 32
 */

int isalpha(int c)
{
	return ((unsigned char) c | 0x20) - 'a' < 26;
}
EXPORT_SYMBOL(isalpha);


/**
 * @brief check if an alphabetic character is in uppercase
 *
 * @param c the character to test
 *
 * @returns 0 if lowercase or out of range
 *
 */

int isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}
EXPORT_SYMBOL(isupper);


/**
 * @brief check if an alphabetic character is in lowercase
 *
 * @param c the character to test
 *
 * @returns 0 if uppercase or out of range
 *
 */

int islower(int c)
{
    return c >= 'a' && c <= 'z';
}
EXPORT_SYMBOL(islower);



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


/**
 * @brief fills a memory area with the constant byte c
 *
 * @param s a pointer to the memory area
 * @param c the byte to set
 * @param n the number of bytes to fill
 *
 * @returns a pointer to the memory area s
 */

void *memset(void *s, int c, size_t n)
{
	size_t i;
	char byte;
	char *p;
	uint64_t cc;

	/* memset accepts only the least significant BYTE to set,
	 * so we first prep by filling our integer to full width
	 * with that byte
	 */
	p    = (char *) &cc;
	byte = (char) (c & 0xFF);
	for (i = 0; i < sizeof(uint64_t); i++)
		p[i] = byte;


	/* now start filling the memory area */
	p = s;


	/* align head */
	while (n) {
		if (IS_ALIGNED((uintptr_t)p, sizeof(uint64_t)))
			break;

		*p++ = (char) (c & 0xFF);
		n--;
	}
	/* do larger stores in the central segment
	 * for arch specific implementations, you could do larger than
	 * int stores with precomputed offsets for increased efficiency
	 * NOTE: doing two stores appears to be more efficient (~7%)
	 * but there is no measurable improvement after that
	 */
	while (n >= sizeof(uint64_t) * 2) {
		uint64_t *dw = (uint64_t *)p;

		dw[0] = cc;
		dw[1] = cc;
		p += sizeof(uint64_t) * 2;
		n -= sizeof(uint64_t) * 2;
	}


	/* fill remaining tail
	 * NOTE: we need the optimisation barrier, else bcc2 will produce
	 * garbage. It appears it wants to re-use the first (mostly indentical)
	 * code segment which aligns the head of the buffer
	 */
	while (n) {
		*p++ = (char) (c & 0xFF);
		barrier();
		n--;
	}

	return s;
}EXPORT_SYMBOL(memset);


/**
 * @brief fills a memory area with the constant uint16_t c
 *
 * @param s a pointer to the memory area
 * @param c the uint16_t to set
 * @param n the number of uint16_t elements to fill
 *
 * @returns a pointer to the memory area s
 */

void *memset16(void *s, uint16_t c, size_t n)
{
        uint16_t *p = s;

        while (n--)
                *p++ = c;

        return s;
}
EXPORT_SYMBOL(memset16);


/**
 * @brief fills a memory area with the constant uint32_t c
 *
 * @param s a pointer to the memory area
 * @param c the uint32_t to set
 * @param n the number of uint32_t elements to fill
 *
 * @returns a pointer to the memory area s
 */

void *memset32(void *s, uint32_t c, size_t n)
{
        uint32_t *p = s;

        while (n--)
                *p++ = c;

        return s;
}
EXPORT_SYMBOL(memset32);


/**
 * @brief convert a string to a long integer
 *
 * @param nptr	the pointer to a string (possibly) representing a number
 * @param endptr if not NULL, stores the pointer to the first character not part
 *		 of the number
 * @param base  the base of the number string
 *
 * @return the converted number
 *
 * @note A base of 0 defaults to 10 unless the first character after
 *	 ' ', '+' or '-' is a '0', which will default it to base 8, unless
 *	 it is followed by 'x' or 'X'  which defaults to base 16.
 *	 If a number is not preceeded by a sign, it is assumed to be a unsigned
 *	 type and may therefore assume the size of ULONG_MAX.
 *	 If no number has been found, the function will return 0 and
 *	 nptr == endptr
 *
 * @note if the value would over-/underflow, LONG_MAX/MIN is returned
 */

long int strtol(const char *nptr, char **endptr, int base)
{
	int neg = 0;

	unsigned long res = 0;
	unsigned long clamp = (unsigned long) ULONG_MAX;



	if (endptr)
		(*endptr) = (char *) nptr;

	for (; isspace(*nptr); nptr++);


	if ((*nptr) == '-') {
		nptr++;
		neg = 1;
		clamp = -(unsigned long) LONG_MIN;
	} else if ((*nptr) == '+') {
		clamp =  (unsigned long) LONG_MAX;
		nptr++;
	}

	if (!base || base == 16) {
		if ((*nptr) == '0') {
			switch (nptr[1]) {
			case 'x':
			case 'X':
				nptr += 2;
				base = 16;
				break;
			default:
				nptr++;
				base = 8;
				break;
			}
		} else {
			/* default */
			base = 10;
		}
	}


	/* Now iterate over the string and add up the digits. We convert
	 * any A-Z to a-z (offset == 32 in ASCII) to check if the string
	 * contains letters representing values (A=10...Z=35). We abort if
	 * the computed digit value exceeds the given base or on overflow.
	 */

	while (1) {
		unsigned int c = (*nptr);
		unsigned int l = c | 0x20;
		unsigned int val;

		if (isdigit(c))
			val = c - '0';
		else if (islower(l))
		       val = l - 'a' + 10;
		else
			break;

		if (val >= base)
			break;

		/* Check for overflow only if result is approximately within
		 * range of it, given the base. If we overflow, set result
		 * to clamp (LONG_MAX or LONG_MIN)
		 */

		if (res & (~0UL << (BITS_PER_LONG - fls(base)))) {
	               if (res > (clamp - val) / base) {
			       res = clamp;
			       break;
		       }
		}

		res = res * base + val;
		nptr++;
	}


	/* update endptr if a value was found */
	if (res)
		if (endptr)
			(*endptr) = (char *) nptr;


	if (neg)
		return - (long int) res;

	return (long int) res;
}


/**
 * @brief convert a string to a long long integer
 *
 * @param nptr	the pointer to a string (possibly) representing a number
 * @param endptr if not NULL, stores the pointer to the first character not part
 *		 of the number
 * @param base  the base of the number string
 *
 * @return the converted number
 *
 * @note A base of 0 defaults to 10 unless the first character after
 *	 ' ', '+' or '-' is a '0', which will default it to base 8, unless
 *	 it is followed by 'x' or 'X'  which defaults to base 16.
 *	 If a number is not preceeded by a sign, it is assumed to be a unsigned
 *	 type and may therefore assume the size of ULLONG_MAX.
 *	 If no number has been found, the function will return 0 and
 *	 nptr == endptr
 *
 * @note if the value would over-/underflow, LLONG_MAX/MIN is returned
 */

long long int strtoll(const char *nptr, char **endptr, int base)
{
	int neg = 0;

	unsigned long long res = 0;
	unsigned long long clamp = (unsigned long) ULONG_LONG_MAX;



	if (endptr)
		(*endptr) = (char *) nptr;

	for (; isspace(*nptr); nptr++);


	if ((*nptr) == '-') {
		nptr++;
		neg = 1;
		clamp = -(unsigned long long) LONG_LONG_MIN;
	} else if ((*nptr) == '+') {
		clamp =  (unsigned long long) LONG_LONG_MAX;
		nptr++;
	}

	if (!base || base == 16) {
		if ((*nptr) == '0') {
			switch (nptr[1]) {
			case 'x':
			case 'X':
				nptr += 2;
				base = 16;
				break;
			default:
				nptr++;
				base = 8;
				break;
			}
		} else {
			/* default */
			base = 10;
		}
	}


	/* Now iterate over the string and add up the digits. We convert
	 * any A-Z to a-z (offset == 32 in ASCII) to check if the string
	 * contains letters representing values (A=10...Z=35). We abort if
	 * the computed digit value exceeds the given base or on overflow.
	 */

	while (1) {
		unsigned int c = (*nptr);
		unsigned int l = c | 0x20;
		unsigned int val;

		if (isdigit(c))
			val = c - '0';
		else if (islower(l))
		       val = l - 'a' + 10;
		else
			break;

		if (val >= base)
			break;

		/* Check for overflow only if result is approximately within
		 * range of it, given the base. If we overflow, set result
		 * to clamp (LLONG_MAX or LLONG_MIN)
		 */

		if (res & (~0ULL << (BITS_PER_LONG_LONG - fls(base)))) {
	               if (res > (clamp - val) / base) {
			       res = clamp;
			       break;
		       }
		}

		res = res * base + val;
		nptr++;
	}


	/* update endptr if a value was found */
	if (res)
		if (endptr)
			(*endptr) = (char *) nptr;


	if (neg)
		return - (long long int) res;

	return (long long int) res;
}

