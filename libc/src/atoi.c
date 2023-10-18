#include <ctype.h>

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


	for (; isspace((int) (*nptr)); ++nptr);

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

