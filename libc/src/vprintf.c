#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <compiler.h>

int vsnprintf(char *str, size_t size, const char *format, va_list ap);


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

