#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <ar.h>




int main(int argc, char **argv)
{
	FILE *f;

	char *buf;
	size_t len;

	struct archive a;
	int ret;


	if (argc < 2) {
		fprintf(stderr, "usage: %s [ar file]\n", argv[0]);
		return 1;
	}

	f = fopen(argv[1], "rb");

	if (!f) {
		perror("opening file");
		return 1;
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);


	buf = (char *) malloc(len + 1);


	if (!buf) {
		perror("malloc");
                fclose(f);
		return 1;
	}

	fread(buf, len, 1, f);
	fclose(f);


	archive_load(buf, len, &a);

	free(buf);
	return 0;
}

