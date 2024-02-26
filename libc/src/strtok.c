#include <string.h>
#include <stddef.h>

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
		s_prev = memchr(token, '\0', 0x7FFFFFFF);
	}

	return token;

}
