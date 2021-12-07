/**
 *
 *  all the functions which are currently stubs
 *
 */

#include <stddef.h>


int printf(const char *fmt, ...);

void exit(int status)
{	/* TODO: syscall */
	(void) status;
	printf("exit() is currently not available on target\n");
	while(1);
}

int fclose(void *stream)
{
	(void) stream;
	printf("fclose() is not available on target\n");

	return 0;
}

void *fopen(const char *pathname, const char *mode)
{
	(void) pathname;
	(void) mode;
	printf("fopen() is not available on target\n");

	return NULL;
}

size_t fread(void *ptr, size_t size, size_t nmemb, void *stream)
{
	(void) ptr;
	(void) size;
	(void) nmemb;
	(void) stream;

	printf("fread() is not available on target\n");

	return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, void *stream)
{
	(void) ptr;
	(void) size;
	(void) nmemb;
	(void) stream;

	printf("fwrite() is not available on target\n");

	return 0;
}

char *fgets(char *s, int size, void *stream)
{
	(void) s;
	(void) size;
	(void) stream;

	printf("fgets() is not available on target\n");

	return NULL;
}

int fseek(void *stream, long offset, int whence)
{
	(void) stream;
	(void) offset;
	(void) whence;

	printf("fseek) is not available on target\n");

	return 0;
}


int sscanf(const char *str, const char *format, ...)
{
	(void) str;
	(void) format;
	printf("sscanf() is not available on target\n");

	return 0;
}


void perror(const char *s)
{
	printf("perror() was called with: %s\n", s);
}
