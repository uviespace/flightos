/**
 * @brief check if an alphabetic character is in uppercase
 *
 * @param c the character to test
 *
 * @returns 0 if lowercase or out of range
 *
 */

int isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}
