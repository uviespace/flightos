/**
 * @brief copy a '\0' terminated string
 *
 * @param dest the destination of the string
 * @param src the source of string
 *
 * @return a pointer to dest
 *
 * @note strings may not overlap
 *
 */

char *strcpy(char *dest, const char *src)
{
	char *tmp;


	tmp = dest;

	while ((*dest++ = *src++) != '\0');

	return tmp;
}
