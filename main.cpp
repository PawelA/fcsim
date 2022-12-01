#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include "fcsim.h"
#include "draw.h"
#include "net.h"

fcsim_block_def blocks[1000];

char *read_file(const char *name)
{
	FILE *fp;
	char *ptr;
	int len;

	fp = fopen(name, "rb");
	if (!fp)
		return NULL;

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

bool check(int cnt)
{
	for (int i = 0; i < cnt; i++) {
		fcsim_block_def *g = &blocks[i];
		if (g->type == FCSIM_WHEEL) {
			for (int j = 0; j < cnt; j++) {
				fcsim_block_def *e = &blocks[j];
				if (e->type == FCSIM_END) {
					if (e->x - e->w/2 > g->x - g->w/2) return false;
					if (e->x + e->w/2 < g->x + g->w/2) return false;
					if (e->y - e->h/2 > g->y - g->h/2) return false;
					if (e->y + e->h/2 < g->y + g->h/2) return false;
					return true;
				}
			}
		}
	}
	return false;
}

int main(void)
{
	char *xml;
	int block_count;
	GLFWwindow *window;

	xml = read_file("level.xml");
	block_count = fcsim_parse_xml(xml, blocks);

	if (!glfwInit())
		return 1;
	//glfwWindowHint(GLFW_VISIBLE, 0);
	window = glfwCreateWindow(800, 800, "fcsim demo", NULL, NULL);
	if (!window)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (minit())
		printf("net disconnected\n");

	//uint64_t x = 0x405df22a8c92047f;
	//mw("parse_check", 1, strtod("119.78384699115303", NULL));
	//mw("parse_check", 1, *(double *)&x, NULL);

	fcsim_create_world();
	for (int i = 0; i < block_count; i++) {
		if (blocks[i].id != -1)
			fcsim_add_block(&blocks[i]);
	}
	for (int i = 0; i < block_count; i++) {
		if (blocks[i].id == -1)
			fcsim_add_block(&blocks[i]);
	}
	fcsim_generate();

	setup_draw();

	int ticks = 0;
	while (true) {
		draw_world(blocks, block_count);
		glfwSwapBuffers(window);
		glfwPollEvents();
		fcsim_step();
		printf("%d\n", ++ticks);
		if (check(block_count))
			break;
	}

	return 0;
}
