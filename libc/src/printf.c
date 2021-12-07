#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

int vprintf(const char *format, va_list ap);


/**
 * @brief format a string and print it to the standard output
 *
 * @param format the format string buffer
 * @param ...    arguments to the format string
 *
 * @return the number of characters written to stdout
 */


int printf(const char *fmt, ...)
{
	int ret;

	va_list args;

	va_start(args, fmt);
	ret = vprintf(fmt, args);
	va_end(args);

	return ret;
}
