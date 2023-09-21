#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include <pthread.h>
#include <fcsim.h>
#include "font.h"

#define TAU 6.28318530718
	
struct fcsim_arena arena;
struct fcsim_handle *handle;
struct fcsim_block_stat *block_stats;
int ticks = 0;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
int running;

void *task(void *arg)
{
	while (1) {
		pthread_mutex_lock(&cond_mutex);
		while (!running) {
			pthread_cond_wait(&cond, &cond_mutex);
		}
		pthread_mutex_unlock(&cond_mutex);

		while (1) {
			/*
			int running_copy;
			pthread_mutex_lock(&running_mutex);
			running_copy = running;
			pthread_mutex_unlock(&running_mutex);
			if (!running_copy)
				break;
			*/
			if (!running)
				break;
			printf("I am a thread!\n");
		}
	}

	return NULL;
}

struct input_box {
	char *buf;
	int len;
	int pos;
};

char design_id_buf[17];

struct input_box design_id_input_box = {
	.buf = design_id_buf,
	.len = 16,
	.pos = 0,
};

void input_box_add_char(struct input_box *input, char c)
{
	int text_len;
	int i;

	text_len = strlen(input->buf);
	if (text_len == input->len)
		return;

	for (i = text_len; i > input->pos; i--)
		input->buf[i] = input->buf[i-1];
	input->buf[input->pos] = c;
	input->pos++;
}

void input_box_backspace(struct input_box *input)
{
	int text_len;
	int i;

	if (input->pos == 0)
		return;
	text_len = strlen(input->buf);

	for (i = input->pos; i <= text_len; i++)
		input->buf[i-1] = input->buf[i];
	input->pos--;
}

void input_box_move_cursor(struct input_box *input, int delta)
{
	int text_len;
	int pos;

	pos = input->pos;
	text_len = strlen(input->buf);
	pos += delta;
	if (pos < 0)
		pos = 0;
	if (pos > text_len)
		pos = text_len;
	input->pos = pos;
}

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

void start(void)
{
}

void stop(void)
{
	handle = fcsim_new(&arena);
	fcsim_get_block_stats(handle, block_stats);
	ticks = 0;
}

void toggle(void)
{
	if (running)
		stop();
	else
		start();
	running = !running;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		toggle();
	if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
		input_box_backspace(&design_id_input_box);
	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		input_box_move_cursor(&design_id_input_box, -1);
	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
		input_box_move_cursor(&design_id_input_box, +1);
}

int width = 800;
int height = 800;

double current_x;
double current_y;

int pressed;

double view_x = 0.0;
double view_y = 0.0;
double view_scale = 1.0;
double view_w_half;
double view_h_half;

int ui_scale = 2;

static void update_view_wh(void)
{
	view_w_half = view_scale * width;
	view_h_half = view_scale * height;
}

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
	double scale = 1 - y * 0.05;

	view_scale *= scale;
	update_view_wh();
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
	if (codepoint >= 32 && codepoint <= 127)
		input_box_add_char(&design_id_input_box, codepoint);
}

static void window_size_callback(GLFWwindow *window, int w, int h)
{
	width = w;
	height = h;
	update_view_wh();
	glViewport(0, 0, w, h);
}

unsigned char font_rgba[20480] = { 0 };

void setup_font(void)
{
	int i;
	int j;

	for (i = 33 * 8; i < 127 * 8; i++) {
		for (j = 0; j < 5; j++) {
			if (font[i - 33 * 8] & (1 << (7 - j))) {
				font_rgba[(i * 5 + j) * 4 + 0] = 0xff;
				font_rgba[(i * 5 + j) * 4 + 1] = 0xff;
				font_rgba[(i * 5 + j) * 4 + 2] = 0xff;
				font_rgba[(i * 5 + j) * 4 + 3] = 0xaa;
			}
		}
	}
}

void vertex2f_pixel(int x, int y)
{
	glVertex2f((float)(2*x - width) / width,
		   (float)(2*y - height) / height);
}

void draw_char(char c, int x, int y, int scale)
{
	int cw = 5 * scale;
	int ch = 8 * scale;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBegin(GL_TRIANGLE_FAN);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0, c / 128.0f); vertex2f_pixel(x, y + ch);
	glTexCoord2f(0, (c + 1) / 128.0f); vertex2f_pixel(x, y);
	glTexCoord2f(1, (c + 1) / 128.0f); vertex2f_pixel(x + cw, y);
	glTexCoord2f(1, c / 128.0f); vertex2f_pixel(x + cw, y + ch);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void draw_box(int x, int y, int w, int h)
{
	glEnable(GL_BLEND);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
	vertex2f_pixel(x, y + h);
	vertex2f_pixel(x, y);
	vertex2f_pixel(x + w, y);
	vertex2f_pixel(x + w, y + h);
	glEnd();
	glDisable(GL_BLEND);
}

void draw_text(char *s, int x, int y, int scale)
{
	int tw = scale * (strlen(s) * 6 + 1);
	int th = scale * 10;
	draw_box(x - scale, y - scale, tw, th);
	for (; *s; s++) {
		draw_char(*s, x, y, scale);
		x += 6 * scale;
	}
}

void draw_text_int(int n, int x, int y, int scale)
{
	char str[20];

	sprintf(str, "%d", n);
	draw_text(str, x, y, scale);
}

void draw_cursor(int x, int y, int scale)
{
	int w = scale;
	int h = scale * 8;

	glEnable(GL_BLEND);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	vertex2f_pixel(x, y + h);
	vertex2f_pixel(x, y);
	vertex2f_pixel(x + w, y);
	vertex2f_pixel(x + w, y + h);
	glEnd();
	glDisable(GL_BLEND);
}

void draw_input_box(struct input_box *input, int x, int y, int scale)
{
	int cx = x;
	int i;

	int tw = scale * (input->len * 6 + 1);
	int th = scale * 10;
	draw_box(x - scale, y - scale, tw, th);

	for (i = 0; i < input->len; i++) {
		draw_char(input->buf[i], cx, y, scale);
		cx += 6 * scale;
	}
	draw_cursor(x + scale * (input->pos * 6 - 1), y, scale);
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

static void draw_rect(struct fcsim_block_def *block,
		      struct fcsim_block_stat *stat,
		      struct color *color)
{
	double sina_half = sin(stat->angle) / 2;
	double cosa_half = cos(stat->angle) / 2;
	double w = fmax(fabs(block->w), 4.0);
	double h = fmax(fabs(block->h), 4.0);
	double wc = w * cosa_half;
	double ws = w * sina_half;
	double hc = h * cosa_half;
	double hs = h * sina_half;
	double x = stat->x;
	double y = stat->y;

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

static void draw_circle(struct fcsim_block_def *block,
			struct fcsim_block_stat *stat,
			struct color *color)
{
	double x = stat->x;
	double y = stat->y;
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
		double a = stat->angle + TAU * i / CIRCLE_SEGMENTS;
		vertex2f_world(cos(a) * r + x, sin(a) * r + y);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(color->r, color->g, color->b);
	vertex2f_world(x, y);
	for (int i = 6; i <= CIRCLE_SEGMENTS; i++) {
		double a = stat->angle + TAU * i / CIRCLE_SEGMENTS;
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

static void draw_block(struct fcsim_block_def *block, struct fcsim_block_stat *stat)
{
	struct color *color = &block_colors[block->type];

	if (is_circle(block))
		draw_circle(block, stat, color);
	else
		draw_rect(block, stat, color);
}

static struct color build_color = { 0.737, 0.859, 0.976 };
static struct color goal_color  = { 0.945, 0.569, 0.569 };

void draw_arena(struct fcsim_arena *arena, struct fcsim_block_stat *stats)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_MULTISAMPLE);
	draw_area(&arena->build, &build_color);
	draw_area(&arena->goal,  &goal_color);
	for (int i = 0; i < arena->block_cnt; i++)
		draw_block(&arena->blocks[i], &stats[i]);
	glDisable(GL_MULTISAMPLE);
}

int main(void)
{
	char *xml;
	GLFWwindow *window;
	pthread_t thread;

	xml = read_file("level.xml");
	if (!xml)
		return 1;
	if (fcsim_read_xml(xml, strlen(xml), &arena)) {
		fprintf(stderr, "failed to parse xml\n");
		return 1;
	}
	block_stats = malloc(sizeof(block_stats[0]) * arena.block_cnt);
	handle = fcsim_new(&arena);
	fcsim_get_block_stats(handle, block_stats);

	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_SAMPLES, 4);
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

	setup_font();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 5, 128 * 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, font_rgba);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	update_view_wh();

	//pthread_create(&thread, NULL, task, NULL);

	while (!glfwWindowShouldClose(window)) {
		draw_arena(&arena, block_stats);
		draw_text_int(ticks, 10, 10, ui_scale);
		draw_input_box(&design_id_input_box, 100, 10, ui_scale);
		glfwSwapBuffers(window);
		glfwPollEvents();
		if (running) {
			fcsim_step(handle);
			fcsim_get_block_stats(handle, block_stats);
			ticks++;
			/*
			if (fcsim_has_won(&arena))
				running = 0;
			*/
		}
		pthread_mutex_lock(&cond_mutex);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&cond_mutex);
	}

	return 0;
}
