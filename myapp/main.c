#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
	char *a;
	int i;
	char hi[] = "Hello World, i am an executable\n";
	char hi2[] = "mxxxxx!!\n";

	printf("meh \n");


	a = calloc(1, strlen(hi) + strlen(hi2) + 100);

	if (!a) {
		printf("fuck, got NULL\n");
		return -1;

	}


	sprintf(a, "%s__%s\n", hi2, hi);

	printf("memory at %p\n", a);
	printf("I'd rather meh this: %s (%d)\n", hi, strlen(hi));
	printf("instead of : %s (%d)\n", hi2, strlen(hi2));
	printf("the sprintffed stuff is %s\n", a);

	printf("now printing memory array with overflow:\n");

	for (i=0; i < strlen(hi) + strlen(hi2) + 104; i++)
		printf("%02x:", a[i]);
	printf("\n");

	return 0xf1;
}
