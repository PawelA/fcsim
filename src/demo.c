#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLFW/glfw3.h>

#include "fcsim.h"
#include "text.h"
#include "ui.h"
#include "arena_layer.h"

int the_width = 800;
int the_height = 800;
int the_cursor_x = 0;
int the_cursor_y = 0;
int the_ui_scale = 4;
void *the_window = NULL;

struct arena_layer the_arena_layer;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	arena_layer_key_event(&the_arena_layer, key, action);
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
	the_cursor_x = x;
	the_cursor_y = y;

	arena_layer_mouse_move_event(&the_arena_layer);
}

void scroll_callback(GLFWwindow *window, double x, double y)
{
	arena_layer_scroll_event(&the_arena_layer, y);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	arena_layer_mouse_button_event(&the_arena_layer, button, action);
}

void window_size_callback(GLFWwindow *window, int w, int h)
{
	the_width = w;
	the_height = h;
	glViewport(0, 0, w, h);

	arena_layer_size_event(&the_arena_layer);
}

int main(void)
{
	GLFWwindow *window;
	int res;

	arena_layer_init(&the_arena_layer);

	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_SAMPLES, 4);
	window = glfwCreateWindow(800, 800, "fcsim demo", NULL, NULL);
	if (!window)
		return 1;
	the_window = window;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	text_setup_draw();
	arena_layer_show(&the_arena_layer);

	while (!glfwWindowShouldClose(window)) {
		arena_layer_draw(&the_arena_layer);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}
