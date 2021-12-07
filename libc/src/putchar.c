/**
 * @brief  writes the character c, cast to an unsigned char, to stdout
 *
 * @param c the character to write
 *
 * @return the number of characters written to buf
 *
 * FIXME: we need a syscall for output
 */

int putchar(int c)
{
#define TREADY 4

#if 1
	static volatile int *console = (int *)0x80000100; /* LEON3 */
#else
        static volatile int *console = (int *)0xFF900000; /* LEON4 */
#endif



	while (!(console[1] & TREADY));

	console[0] = 0x0ff & c;

	if (c == '\n') {
		while (!(console[1] & TREADY));
		console[0] = (int) '\r';
	}

	return c;
}




