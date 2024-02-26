#include <stddef.h>

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
