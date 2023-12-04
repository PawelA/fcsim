#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <fcsim.h>

#include "text.h"
#include "ui.h"
#include "loader.h"
#include "event.h"
#include "load_layer.h"
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
	struct event event;

	event.type = EVENT_KEY;
	event.key_event.key = key;
	event.key_event.action = action;

	arena_layer_event(&the_arena_layer, &event);
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
	struct event event;

	event.type = EVENT_MOUSE_MOVE;

	the_cursor_x = x;
	the_cursor_y = y;

	arena_layer_event(&the_arena_layer, &event);
}

void scroll_callback(GLFWwindow *window, double x, double y)
{
	struct event event;

	event.type = EVENT_SCROLL;
	event.scroll_event.delta = y;

	arena_layer_event(&the_arena_layer, &event);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	struct event event;

	event.type = EVENT_MOUSE_BUTTON;
	event.mouse_button_event.button = button;
	event.mouse_button_event.action = action;

	arena_layer_event(&the_arena_layer, &event);
}

void char_callback(GLFWwindow *window, unsigned int codepoint)
{
	struct event event;

	event.type = EVENT_CHAR;
	event.char_event.codepoint = codepoint;

	arena_layer_event(&the_arena_layer, &event);
}

void window_size_callback(GLFWwindow *window, int w, int h)
{
	struct event event;

	event.type = EVENT_SIZE;

	the_width = w;
	the_height = h;
	glViewport(0, 0, w, h);

	arena_layer_event(&the_arena_layer, &event);
}

static const char* vertex_shader_text =
	"#version 110\n"
	"attribute vec2 pos;"
	"void main()"
	"{"
		"gl_Position = vec4(pos, 0.0, 1.0);"
	"}";

static const char* fragment_shader_text =
	"#version 110\n"
	"void main()"
	"{"
		"gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
	"}";

float vertices[] = {
	-0.6f, -0.4f,
	 0.6f, -0.4f,
	 0.0f,  0.6f,
};

void draw_triangle(void)
{
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;
	GLuint vertex_buffer;
	GLuint pos_location;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
	glCompileShader(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
	glCompileShader(fragment_shader);

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glUseProgram(program);

	pos_location = glGetAttribLocation(program, "pos");

	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(pos_location);
	glVertexAttribPointer(pos_location, 2, GL_FLOAT, GL_FALSE,
			      0, (void*) 0);
}

/*
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}
*/

int main(void)
{
	GLFWwindow *window;
	int res;

	loader_init();
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

	gladLoadGLES2Loader(glfwGetProcAddress);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCharCallback(window, char_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	/*
	text_setup_draw();
	arena_layer_show(&the_arena_layer);
	*/

	draw_triangle();

	while (!glfwWindowShouldClose(window)) {
		//arena_layer_draw(&the_arena_layer);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}
