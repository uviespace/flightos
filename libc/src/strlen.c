#include <stddef.h>

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
