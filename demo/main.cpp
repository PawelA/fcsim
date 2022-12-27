#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <fcsim.h>
#include <net.h>
#include "draw.h"

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

/*
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
*/

int main()
{
	char *xml;
	fcsim_arena arena;
	fcsim_handle *handle;
	GLFWwindow *window;

	xml = read_file("level.xml");
	fcsim_read_xml(xml, &arena);
	handle = fcsim_new(&arena);

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

	//mw("parse_check", 1, strtod("119.78384699115303", NULL));

	setup_draw();

	int ticks = 0;
	while (!glfwWindowShouldClose(window)) {
		draw_world(arena.blocks, arena.block_cnt);
		glfwSwapBuffers(window);
		glfwPollEvents();
		fcsim_step(handle);
		printf("%d\n", ++ticks);
	}

	return 0;
}
