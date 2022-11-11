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
	glClearColor(1, 1, 1, 0);
	glOrtho(-500, 500, 500, -500, -1, 1);
}

static void draw_rect(struct fcsim_block *block, float r, float g, float b)
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
	glColor3f(r, g, b);
	glVertex2f( wc - hs + x,  ws + hc + y);
	glVertex2f(-wc - hs + x, -ws + hc + y);
	glVertex2f(-wc + hs + x, -ws - hc + y);
	glVertex2f( wc + hs + x,  ws - hc + y);
	glEnd();
}

static void draw_circle(struct fcsim_block *block, bool wheel, float r, float g, float b)
{
	int i;
	float ra = block->w/2;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(r, g, b);
	for (i = 0; i < 40; i++) {
		glVertex2f(cosf(6.28 * i / 40.0) * ra + block->x,
			   sinf(6.28 * i / 40.0) * ra + block->y);
	}
	glEnd();
}

static void draw_block(struct fcsim_block *block)
{
	switch (block->type) {
	case FCSIM_STAT_RECT:
		draw_rect(block, 0, 0.9, 0);
		break;
	case FCSIM_STAT_CIRCLE:
		draw_circle(block, false, 0, 0.9, 0);
		break;
	case FCSIM_DYN_RECT:
		draw_rect(block, 1, 0.8, 0);
		break;
	case FCSIM_DYN_CIRCLE:
		draw_circle(block, false, 1, 0.5, 0);
		break;
	case FCSIM_GOAL_CIRCLE:
		draw_circle(block, true, 1, 0, 0.1);
		break;
	case FCSIM_GOAL_RECT:
		draw_rect(block, 1, 0, 0.1);
		break;
	case FCSIM_WHEEL:
		draw_circle(block, true, 0, 0.6, 1);
		break;
	case FCSIM_CW_WHEEL:
		draw_circle(block, true, 1, 0.9, 0);
		break;
	case FCSIM_CCW_WHEEL:
		draw_circle(block, true, 0.8, 0, 0.9);
		break;
	case FCSIM_ROD:
		block->h = 4;
		draw_rect(block, 0, 0, 1);
		break;
	case FCSIM_SOLID_ROD:
		block->h = 8;
		draw_rect(block, 0.5, 0.3, 0);
		break;
	}
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
