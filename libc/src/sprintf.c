#include <stddef.h>
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
