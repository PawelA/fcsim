#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include <fcsim.h>

#define TAU 6.28318530718

static char *read_file(const char *name)
{
	FILE *fp;
	char *ptr;
	int len;

	fp = fopen(name, "rb");
	if (!fp) {
		perror(name);
		exit(1);
	}

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

int running;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		running = !running;
}

int width = 800;
int height = 800;

double current_x;
double current_y;

int pressed;

double view_x = 0.0;
double view_y = 0.0;
double view_w_half = 500.0;
double view_h_half = 500.0;


static void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
	double prev_x = current_x;
	double prev_y = current_y;

	if (pressed) {
		double dx_pixel = x - prev_x;
		double dy_pixel = y - prev_y;
		double dx_world = (dx_pixel / width) * view_w_half * 2;
		double dy_world = (dy_pixel / height) * view_h_half * 2;
		view_x -= dx_world;
		view_y -= dy_world;
	}

	current_x = x;
	current_y = y;
}

static void scroll_callback(GLFWwindow *window, double x, double y)
{
	double scale = 1 + y * 0.01;
	view_w_half *= scale;
	view_h_half *= scale;
}

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	if (action == GLFW_PRESS) {
		pressed = 1;
	} else {
		pressed = 0;
	}
}

static void char_callback(GLFWwindow *window, unsigned int codepoint)
{
	printf("char: %u\n", codepoint);
}

static void window_size_callback(GLFWwindow *window, int width, int height)
{
	printf("size: %d %d\n", width, height);
}

struct color {
	float r;
	float g;
	float b;
};

struct color block_colors[] = {
	{ 0.000f, 0.745f, 0.004f }, /* FCSIM_STAT_RECT */
	{ 0.000f, 0.745f, 0.004f }, /* FCSIM_STAT_CIRCLE */
	{ 0.976f, 0.855f, 0.184f }, /* FCSIM_DYN_RECT */
	{ 0.976f, 0.537f, 0.184f }, /* FCSIM_DYN_CIRCLE */
	{ 1.000f, 0.400f, 0.400f }, /* FCSIM_GOAL_RECT */
	{ 1.000f, 0.400f, 0.400f }, /* FCSIM_GOAL_CIRCLE */
	{ 0.537f, 0.980f, 0.890f }, /* FCSIM_WHEEL */
	{ 1.000f, 0.925f, 0.000f }, /* FCSIM_CW_WHEEL */
	{ 1.000f, 0.800f, 0.800f }, /* FCSIM_CCW_WHEEL */
	{ 0.000f, 0.000f, 1.000f }, /* FCSIM_ROD */
	{ 0.420f, 0.204f, 0.000f }, /* FCSIM_SOLID_ROD */
};

void vertex2f_world(double x, double y)
{
	glVertex2f((x - view_x) / view_w_half, (view_y - y) / view_h_half);
}

static void draw_rect(struct fcsim_block_def *block, struct color *color)
{
	double sina_half = sin(block->angle) / 2;
	double cosa_half = cos(block->angle) / 2;
	double w = fmax(block->w, 4.0);
	double h = fmax(block->h, 4.0);
	double wc = w * cosa_half;
	double ws = w * sina_half;
	double hc = h * cosa_half;
	double hs = h * sina_half;
	double x = block->x;
	double y = block->y;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(color->r, color->g, color->b);
	vertex2f_world( wc - hs + x,  ws + hc + y);
	vertex2f_world(-wc - hs + x, -ws + hc + y);
	vertex2f_world(-wc + hs + x, -ws - hc + y);
	vertex2f_world( wc + hs + x,  ws - hc + y);
	glEnd();
}

static void draw_area(struct fcsim_rect *area, struct color *color)
{
	double w_half = area->w / 2;
	double h_half = area->h / 2;
	double x = area->x;
	double y = area->y;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(color->r, color->g, color->b);
	vertex2f_world(x + w_half, y + h_half);
	vertex2f_world(x + w_half, y - h_half);
	vertex2f_world(x - w_half, y - h_half);
	vertex2f_world(x - w_half, y + h_half);
	glEnd();
}

#define CIRCLE_SEGMENTS 32
#define COLOR_SCALE 0.85

static void draw_circle(struct fcsim_block_def *block, struct color *color)
{
	double x = block->x;
	double y = block->y;
	float r;

	if (block->type == FCSIM_DYN_CIRCLE || block->type == FCSIM_STAT_CIRCLE)
		r = block->w;
	else
		r = block->w/2;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(color->r * COLOR_SCALE,
		  color->g * COLOR_SCALE,
		  color->b * COLOR_SCALE);
	vertex2f_world(x, y);
	for (int i = 0; i <= 6; i++) {
		double a = block->angle + TAU * i / CIRCLE_SEGMENTS;
		vertex2f_world(cos(a) * r + x, sin(a) * r + y);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(color->r, color->g, color->b);
	vertex2f_world(x, y);
	for (int i = 6; i <= CIRCLE_SEGMENTS; i++) {
		double a = block->angle + TAU * i / CIRCLE_SEGMENTS;
		vertex2f_world(cos(a) * r + x, sin(a) * r + y);
	}
	glEnd();
}

static int is_circle(struct fcsim_block_def *block)
{
	switch (block->type) {
	case FCSIM_STAT_CIRCLE:
	case FCSIM_DYN_CIRCLE:
	case FCSIM_GOAL_CIRCLE:
	case FCSIM_WHEEL:
	case FCSIM_CW_WHEEL:
	case FCSIM_CCW_WHEEL:
		return 1;
	}

	return 0;
}

static void draw_block(struct fcsim_block_def *block)
{
	struct color *color = &block_colors[block->type];

	if (is_circle(block))
		draw_circle(block, color);
	else
		draw_rect(block, color);
}

static struct color build_color = { 0.737, 0.859, 0.976 };
static struct color goal_color  = { 0.945, 0.569, 0.569 };

void draw_arena(struct fcsim_arena *arena)
{
	glClear(GL_COLOR_BUFFER_BIT);
	draw_area(&arena->build, &build_color);
	draw_area(&arena->goal,  &goal_color);
	for (int i = 0; i < arena->block_cnt; i++)
		draw_block(&arena->blocks[i]);
}

int main(void)
{
	char *xml;
	struct fcsim_arena arena;
	struct fcsim_handle *handle;
	GLFWwindow *window;

	xml = read_file("level.xml");
	if (!xml)
		return 1;
	if (fcsim_read_xml(xml, &arena)) {
		fprintf(stderr, "failed to parse xml\n");
		return 1;
	}
	handle = fcsim_new(&arena);

	if (!glfwInit())
		return 1;
	window = glfwCreateWindow(800, 800, "fcsim demo", NULL, NULL);
	if (!window)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCharCallback(window, char_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	glClearColor(0.529f, 0.741f, 0.945f, 0);

	int ticks = 0;
	while (!glfwWindowShouldClose(window)) {
		draw_arena(&arena);
		glfwSwapBuffers(window);
		glfwPollEvents();
		if (running) {
			fcsim_step(handle);
			printf("%d\n", ++ticks);
			if (fcsim_has_won(&arena))
				running = 0;
		}
	}

	return 0;
}
