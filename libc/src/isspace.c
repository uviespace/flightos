/**
 * @brief check if a character is a white space
 *
 * @param c the character to test
 *
 * @returns 0 if not a space
 *
 * @note c must have the value of an unsigned char or EOF
 *
 */

int isspace(int c)
{
	if (c == ' ')
		return 1;

	if (c == '\t')
		return 1;

	if (c == '\n')
		return 1;

	if (c == '\v')
		return 1;

        if (c == '\f')
		return 1;

       	if (c == '\r')
		return 1;


	return 0;
}
