/* we don't use ctype-style bitfields but rather
 * explicit functions so we have to prototype them here
 * for internal use within the libc
 */

int isupper(int c);
int isspace(int c);
int islower(int c);
int isdigit(int c);
int isalpha(int c);
