#include <stddef.h>

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
