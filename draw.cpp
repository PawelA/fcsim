#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fcsim.h"
	
GLFWwindow* window;

/*
int create_window(int w, int h, const char *title)
{
	if (!glfwInit())
		return -1;
 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
 
	window = glfwCreateWindow(w, h, title, NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
 
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glClearColor(1, 1, 1, 0);
	glOrtho(-400, 400, 400, -400, -1, 1);

	return 0;
}
int destroy_window(void)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

int window_should_close(void)
{
	return glfwWindowShouldClose(window);
}
*/

void setup_draw(void)
{
	glClearColor(0.529f, 0.741f, 0.945f, 0);
	glOrtho(-500, 500, 500, -500, -1, 1);
}

struct draw_info {
	int circle;
	float r, g, b;
};

draw_info draw_info_tbl[] = {
	{ 0, 0.000f, 0.745f, 0.004f }, /* FCSIM_STAT_RECT */
	{ 1, 0.000f, 0.745f, 0.004f }, /* FCSIM_STAT_CIRCLE */
	{ 0, 0.976f, 0.855f, 0.184f }, /* FCSIM_DYN_RECT */
	{ 1, 0.976f, 0.537f, 0.184f }, /* FCSIM_DYN_CIRCLE */
	{ 0, 1.000f, 0.400f, 0.400f }, /* FCSIM_GOAL_RECT */
	{ 1, 1.000f, 0.400f, 0.400f }, /* FCSIM_GOAL_CIRCLE */
	{ 1, 0.537f, 0.980f, 0.890f }, /* FCSIM_WHEEL */
	{ 1, 1.000f, 0.925f, 0.000f }, /* FCSIM_CW_WHEEL */
	{ 1, 1.000f, 0.800f, 0.800f }, /* FCSIM_CCW_WHEEL */
	{ 0, 0.000f, 0.000f, 1.000f }, /* FCSIM_ROD */
	{ 0, 0.420f, 0.204f, 0.000f }, /* FCSIM_SOLID_ROD */
};

static void draw_rect(struct fcsim_block *block, float cr, float cg, float cb)
{
	float sina = sinf(block->angle);
	float cosa = cosf(block->angle);
	float wc = block->w * cosa / 2;
	float ws = block->w * sina / 2;
	float hc = block->h * cosa / 2;
	float hs = block->h * sina / 2;
	float x = block->x;
	float y = block->y;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(cr, cg, cb);
	glVertex2f( wc - hs + x,  ws + hc + y);
	glVertex2f(-wc - hs + x, -ws + hc + y);
	glVertex2f(-wc + hs + x, -ws - hc + y);
	glVertex2f( wc + hs + x,  ws - hc + y);
	glEnd();
}

#define CIRCLE_SEGMENTS 40

static void draw_circle(struct fcsim_block *block, float cr, float cg, float cb)
{
	int i;
	float r = block->w/2;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(cr, cg, cb);
	for (i = 0; i < CIRCLE_SEGMENTS; i++) {
		glVertex2f(cosf(6.28 * i / CIRCLE_SEGMENTS) * r + block->x,
			   sinf(6.28 * i / CIRCLE_SEGMENTS) * r + block->y);
	}
	glEnd();
}

static void draw_block(struct fcsim_block *block)
{
	draw_info info = draw_info_tbl[block->type];
	if (info.circle)
		draw_circle(block, info.r, info.g, info.b);
	else
		draw_rect(block, info.r, info.g, info.b);
}
 
void draw_world(struct fcsim_block *blocks, int block_cnt)
{
	int i;

	glClear(GL_COLOR_BUFFER_BIT);
	for (i = 0; i < block_cnt; i++)
		draw_block(&blocks[i]);
	/*
	glfwSwapBuffers(window);
	glfwPollEvents();
	*/
}
