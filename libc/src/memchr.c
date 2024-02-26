#include <stddef.h>

/**
 * @brief	scan memory for a character
 * @param s	the memory area
 * @param c	the byte to search
 * @param n	The size of the area
 *
 * @returns	the address of the first occurrence of c or NULL if not found
 */

void *memchr(const void *s, int c, size_t n)
{
        const unsigned char *p = s;


        while (n--) {

                if ((unsigned char)c == *p++)
                        return (void *)(p - 1);
        }

        return NULL;
}
