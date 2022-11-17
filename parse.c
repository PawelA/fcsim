#include <stdio.h>
#include "yxml/yxml.h"

char buf[4096];

int main(void)
{
	yxml_t yxml;
	FILE *file;
	int nread;
	char c;

	yxml_init(&yxml, buf, sizeof(buf));

	file = fopen("level.xml", "rb");
	if (!file)
		return 1;

	while ((c = fgetc(file)) != EOF) {
		yxml_ret_t ret = yxml_parse(&yxml, c);
		switch (ret) {
		case YXML_ELEMSTART:
			printf("elem %s\n", yxml.elem);
			break;
		case YXML_CONTENT:
			printf("%c\n", yxml.data[0]);
			break;
		}
	}

	fclose(file);

	return 0;
}
