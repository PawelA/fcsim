#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <fcsim.h>
#include <net.h>
#include "draw.h"

static char *read_file(const char *name)
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

void print_area(fcsim_rect *area, const char *name)
{
	printf("%s: x = %lf y = %lf w = %lf h = %lf\n", name, area->x, area->y, area->w, area->h);
}

void print_block(fcsim_block_def *block)
{
	printf("type = %d id = %d x = %lf y = %lf w = %lf h = %lf angle = %lf joints = [", block->type, block->id, block->x, block->y, block->w, block->h, block->angle);
	for (int i = 0; i < block->joint_cnt; i++) {
		printf("%d", block->joints[i]);
		if (i + 1 < block->joint_cnt)
			printf(" ");
	}
	printf("]\n");
}

void print_arena(fcsim_arena *arena)
{
	for (int i = 0; i < arena->block_cnt; i++)
		print_block(&arena->blocks[i]);
	print_area(&arena->build, "build");
	print_area(&arena->goal, "goal");
}

int running;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		running = !running;
}

int main()
{
	char *xml;
	fcsim_arena arena;
	fcsim_handle *handle;
	GLFWwindow *window;

	xml = read_file("level.xml");
	if (fcsim_read_xml(xml, &arena))
		return 1;
	print_arena(&arena);
	handle = fcsim_new(&arena);
	minit("127.0.0.1", 2137);

	if (!glfwInit())
		return 1;
	window = glfwCreateWindow(800, 800, "fcsim demo", NULL, NULL);
	if (!window)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, key_callback);

	setup_draw();

	int ticks = 0;
	while (!glfwWindowShouldClose(window)) {
		draw_world(&arena);
		glfwSwapBuffers(window);
		glfwPollEvents();
		if (running) {
			fcsim_step(handle);
			printf("%d\n", ++ticks);
		}
	}

	return 0;
}
