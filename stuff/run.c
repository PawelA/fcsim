#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcsim.h>

static char *read_file(const char *name)
{
	FILE *fp;
	char *ptr;
	int len;

	fp = fopen(name, "rb");
	if (!fp) {
		perror(name);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	ptr = (char *)malloc(len + 1);
	if (ptr) {
		fseek(fp, 0, SEEK_SET);
		fread(ptr, 1, len, fp);
		ptr[len] = 0;
	}
	fclose(fp);

	return ptr;
}

int main(int argc, char **argv)
{
	char *xml;
	struct fcsim_arena arena;
	struct fcsim_handle *handle;
	int ticks = 0;
	int cp_interval = 0;
	int cp_counter = 0;

	if (argc != 2 && argc != 3) {
		fprintf(stderr, "usage: run <xml file> [checkpoint interval]\n");
		return 1;
	}

	xml = read_file(argv[1]);
	if (!xml)
		return 1;

	if (argc == 3)
		cp_interval = atoi(argv[2]);

	if (fcsim_read_xml(xml, strlen(xml), &arena)) {
		fprintf(stderr, "failed to parse xml\n");
		return 1;
	}

	handle = fcsim_new(&arena);

	do {
		fcsim_step(handle);
		ticks++;
		cp_counter++;
		if (cp_counter == cp_interval) {
			printf("* %d\n", ticks);
			cp_counter = 0;
		}
	} while (!fcsim_has_won(&arena));

	printf("%d\n", ticks);

	return 0;
}
