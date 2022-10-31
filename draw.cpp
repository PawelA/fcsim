#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fcsim.h"
#include "fc.h"
	
GLFWwindow* window;

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

void draw_rect(struct fc_rect *rect, float r, float g, float b)
{
	float sina = sinf(rect->body.angle);
	float cosa = cosf(rect->body.angle);
	float wc = rect->w * cosa / 2;
	float ws = rect->w * sina / 2;
	float hc = rect->h * cosa / 2;
	float hs = rect->h * sina / 2;
	float x = rect->body.x;
	float y = rect->body.y;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(r, g, b);
	glVertex2f(wc - hs + x, ws + hc + y);
	glVertex2f(-wc - hs + x, -ws + hc + y);
	glVertex2f(-wc + hs + x, -ws - hc + y);
	glVertex2f(wc + hs + x, ws - hc + y);
	/*
	glVertex2f(-rect->w * cosa - rect->h * sina + rect->body.x,
		   -rect->w * sina + rect->h * cosa + rect->body.y);
	glVertex2f(-rect->w * cosa + rect->h * sina + rect->body.x,
		   -rect->w * sina - rect->h * cosa + rect->body.y);
	glVertex2f(rect->w * cosa + rect->h * sina + rect->body.x,
		   rect->w * sina - rect->h * cosa + rect->body.y);
	*/
	glEnd();
}

void draw_circ(struct fc_circ *circ, float r, float g, float b)
{
	int i;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(r, g, b);
	for (i = 0; i < 20; i++) {
		glVertex2f(cosf(6.28 * i / 20.0) * circ->r + circ->body.x,
			   sinf(6.28 * i / 20.0) * circ->r + circ->body.y);
	}
	glEnd();
}

void draw_world(struct fc_world *world)
{
	int i;

	glClear(GL_COLOR_BUFFER_BIT);

	for (i = 0; i < world->stat_rect_cnt; i++)
		draw_rect(&world->stat_rects[i], 0, 0.9, 0);

	for (i = 0; i < world->stat_circ_cnt; i++)
		draw_circ(&world->stat_circs[i], 0, 0.9, 0);

	for (i = 0; i < world->dyn_rect_cnt; i++)
		draw_rect(&world->dyn_rects[i], 1, 0.9, 0);
	
	for (i = 0; i < world->dyn_circ_cnt; i++)
		draw_circ(&world->dyn_circs[i], 1, 0.4, 0);
 
	glfwSwapBuffers(window);
	glfwPollEvents();
}
