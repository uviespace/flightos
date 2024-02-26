#include <stddef.h>


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
