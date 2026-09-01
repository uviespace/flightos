/**
 * @file include/kernel/string.h
 *
 * @ingroup string
 */

#ifndef _KERNEL_STRING_H_
#define _KERNEL_STRING_H_

#include <kernel/types.h>
#include <stdarg.h>
#include <limits.h>


/**
 * @brief format a string and write it to a buffer
 * @param str: destination buffer
 * @param format: printf-style format string
 * @return number of characters written
 */
int sprintf(char *str, const char *format, ...);

/**
 * @brief format a string with size limit and write it to a buffer
 * @param str: destination buffer
 * @param size: maximum number of characters to write
 * @param format: printf-style format string
 * @return number of characters that would have been written (excluding null)
 */
int snprintf(char *str, size_t size, const char *format, ...);

/**
 * @brief compare two null-terminated strings
 * @param s1: first string
 * @param s2: second string
 * @return 0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief compare up to n characters of two strings
 * @param s1: first string
 * @param s2: second string
 * @param n: maximum number of characters to compare
 * @return 0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int strncmp(const char *s1, const char *s2, size_t n);

/**
 * @brief find first occurrence of any character from accept in s
 * @param s: string to search
 * @param accept: set of acceptable characters
 * @return pointer to first match, or NULL if none found
 */
char *strpbrk(const char *s, const char *accept);

/**
 * @brief extract a token from a string using a delimiter
 * @param stringp: pointer to the string to tokenize (modified in place)
 * @param delim: delimiter string
 * @return pointer to the next token, or NULL if no more tokens
 */
char *strsep(char **stringp, const char *delim);

/**
 * @brief duplicate a string using dynamic allocation
 * @param s: null-terminated string to duplicate
 * @return pointer to newly allocated string, or NULL on failure
 */
char *strdup(const char *s);

/**
 * @brief get length of initial segment matching accept characters
 * @param s: string to measure
 * @param accept: set of acceptable characters
 * @return number of leading characters matching any in accept
 */
size_t strspn(const char *s, const char *accept);

/**
 * @brief tokenize a string using a delimiter
 * @param s: string to tokenize (NULL to continue previous tokenization)
 * @param delim: delimiter string
 * @return pointer to the next token, or NULL if no more tokens
 */
char *strtok(char *s, const char *delim);

/**
 * @brief find first occurrence of a character in a string
 * @param s: null-terminated string to search
 * @param c: character to find
 * @return pointer to first occurrence, or NULL if not found
 */
char *strchr(const char *s, int c);

/**
 * @brief find first occurrence of a substring in a string
 * @param haystack: null-terminated string to search
 * @param needle: null-terminated string to find
 * @return pointer to first occurrence, or NULL if not found
 */
char *strstr(const char *haystack, const char *needle);

/**
 * @brief get the length of a null-terminated string
 * @param s: null-terminated string
 * @return number of characters (excluding null terminator)
 */
size_t strlen(const char *s);

/**
 * @brief compare two memory blocks
 * @param s1: first memory block
 * @param s2: second memory block
 * @param n: number of bytes to compare
 * @return 0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int memcmp(const void *s1, const void *s2, size_t n);

/**
 * @brief fill memory with a constant byte
 * @param s: destination memory block
 * @param c: byte value to fill with
 * @param n: number of bytes to fill
 * @return pointer to destination memory
 */
void *memset(void *s, int c, size_t n);

/**
 * @brief copy memory from source to destination
 * @param dest: destination memory block
 * @param src: source memory block
 * @param n: number of bytes to copy
 * @return pointer to destination memory
 */
void *memcpy(void *dest, const void *src, size_t n);

/**
 * @brief copy memory with overlap handling
 * @param dest: destination memory block
 * @param src: source memory block
 * @param n: number of bytes to copy
 * @return pointer to destination memory
 */
void *memmove(void *dest, const void *src, size_t n);

/**
 * @brief find first occurrence of a byte in a memory block
 * @param s: memory block to search
 * @param c: byte value to find
 * @param n: number of bytes to search
 * @return pointer to first occurrence, or NULL if not found
 */
void *memchr(const void *s, int c, size_t n);

/**
 * @brief copy a null-terminated string
 * @param dest: destination string buffer
 * @param src: source null-terminated string
 * @return pointer to destination string
 */
char *strcpy(char *dest, const char *src);

/**
 * @brief zero out a memory block
 * @param s: memory block to zero
 * @param n: number of bytes to zero
 */
void bzero(void *s, size_t n);

/**
 * @brief fill memory with 16-bit values
 * @param s: destination memory block
 * @param c: 16-bit value to fill with
 * @param n: number of 16-bit elements to fill
 * @return pointer to destination memory
 */
void *memset16(void *s, uint16_t c, size_t n);

/**
 * @brief fill memory with 32-bit values
 * @param s: destination memory block
 * @param c: 32-bit value to fill with
 * @param n: number of 32-bit elements to fill
 * @return pointer to destination memory
 */
void *memset32(void *s, uint32_t c, size_t n);

/**
 * @brief check if a character is a decimal digit
 * @param c: character to test
 * @return non-zero if c is a digit, 0 otherwise
 */
int isdigit(int c);

/**
 * @brief check if a character is a whitespace
 * @param c: character to test
 * @return non-zero if c is whitespace, 0 otherwise
 */
int isspace(int c);

/**
 * @brief check if a character is an alphabetic letter
 * @param c: character to test
 * @return non-zero if c is alphabetic, 0 otherwise
 */
int isalpha(int c);

/**
 * @brief check if a character is an uppercase letter
 * @param c: character to test
 * @return non-zero if c is uppercase, 0 otherwise
 */
int isupper(int c);

/**
 * @brief check if a character is a lowercase letter
 * @param c: character to test
 * @return non-zero if c is lowercase, 0 otherwise
 */
int islower(int c);

/**
 * @brief convert a string to an integer
 * @param nptr: null-terminated string to convert
 * @return converted integer value
 */
int atoi(const char *nptr);

/**
 * @brief convert a string to a long integer with base detection
 * @param nptr: null-terminated string to convert
 * @param endptr: pointer to store the first unconverted character (or NULL)
 * @param base: number base (0 for auto-detection)
 * @return converted long integer value
 */
long int strtol(const char *nptr, char **endptr, int base);

/**
 * @brief convert a string to a long long integer with base detection
 * @param nptr: null-terminated string to convert
 * @param endptr: pointer to store the first unconverted character (or NULL)
 * @param base: number base (0 for auto-detection)
 * @return converted long long integer value
 */
long long int strtoll(const char *nptr, char **endptr, int base);

/**
 * @brief print formatted output to stdout using a va_list
 * @param format: printf-style format string
 * @param ap: va_list of arguments
 * @return number of characters printed, or negative error code
 */
int vprintf(const char *format, va_list ap);

/**
 * @brief format a string using a va_list and write to a buffer
 * @param str: destination buffer
 * @param format: printf-style format string
 * @param ap: va_list of arguments
 * @return number of characters written
 */
int vsprintf(char *str, const char *format, va_list ap);

/**
 * @brief format a string with size limit using a va_list
 * @param str: destination buffer
 * @param size: maximum number of characters to write
 * @param format: printf-style format string
 * @param ap: va_list of arguments
 * @return number of characters that would have been written (excluding null)
 */
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/**
 * @brief write a string to stdout
 * @param s: null-terminated string to output
 * @return non-negative on success, EOF on error
 */
int puts(const char *s);

/**
 * @brief write a single character to stdout
 * @param c: character to output
 * @return the character written, or EOF on error
 */
int putchar(int c);

#endif /* _KERNEL_STRING_H_ */
