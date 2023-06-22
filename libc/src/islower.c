/**
 * @brief check if an alphabetic character is in lowercase
 *
 * @param c the character to test
 *
 * @returns 0 if uppercase or out of range
 *
 */

int islower(int c)
{
    return c >= 'a' && c <= 'z';
}
