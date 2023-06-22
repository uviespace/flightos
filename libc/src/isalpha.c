/**
 * @brief check if a character is from the alphabet
 *
 * @param c the character to test
 *
 * @returns 0 if not in the alphabet
 *
 * @note in ASCII, 'a-z' are 'A-Z' + 32
 */

int isalpha(int c)
{
	return ((unsigned char) c | 0x20) - 'a' < 26;
}
