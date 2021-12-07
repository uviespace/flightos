#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <compiler.h>

int vsnprintf(char *str, size_t size, const char *format, va_list ap);

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
