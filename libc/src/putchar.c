#include <syscalls.h>


/**
 * @brief  writes the character c, cast to an unsigned char, to stdout
 *
 * @param c the character to write
 *
 * @return the number of characters written to buf
 */

int putchar(int c)
{
	char o = (char) (c & 0xff);

	return sys_write(1, (void *) &o, 1);
}
