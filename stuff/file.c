#include <stdio.h>
#include <stdlib.h>
#include "file.h"

int read_file(struct file_buf *buf, const char *filename)
{
	FILE *fp;
	char *ptr;
	int len;

	fp = fopen(filename, "rb");
	if (!fp)
		return -1;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	ptr = malloc(len);
	if (!ptr) {
		fclose(fp);
		return -1;
	}
	fseek(fp, 0, SEEK_SET);
	fread(ptr, 1, len, fp);
	fclose(fp);

	buf->ptr = ptr;
	buf->len = len;

	return 0;
}
