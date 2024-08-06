#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <box2d/b2Body.h>

#include "gl.h"
#include "xml.h"
#include "interval.h"
#include "globals.h"
#include "graph.h"
#include "arena.h"
#include "galois.h"

#define TAU 6.28318530718

const GLchar *vertex_shader_src =
	"attribute vec2 a_coords;"
	"attribute vec3 a_color;"
	"varying vec3 v_color;"
	"void main() {"
		"gl_Position = vec4(a_coords, 0.0, 1.0);"
		"v_color = a_color;"
	"}";

const GLchar *fragment_shader_src =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"#endif\n"
	"varying vec3 v_color;"
	"void main() {"
		"gl_FragColor = vec4(v_color, 1.0);\n"
	"}\n";

float coords[2048];
float colors[2048];

unsigned short indices[2048];

int cnt_index;
int cnt_coord;
int cnt_color;

struct color {
	float r;
	float g;
	float b;
};

struct color block_colors[] = {
	{ 0.000f, 0.745f, 0.004f }, // FCSIM_STAT_RECT
	{ 0.976f, 0.855f, 0.184f }, // FCSIM_DYN_RECT
	{ 0.000f, 0.745f, 0.004f }, // FCSIM_STAT_CIRCLE
	{ 0.976f, 0.537f, 0.184f }, // FCSIM_DYN_CIRCLE
	{ 1.000f, 0.400f, 0.400f }, // FCSIM_GOAL_RECT
	{ 1.000f, 0.400f, 0.400f }, // FCSIM_GOAL_CIRCLE
	{ 0.537f, 0.980f, 0.890f }, // FCSIM_WHEEL
	{ 1.000f, 0.925f, 0.000f }, // FCSIM_CW_WHEEL
	{ 1.000f, 0.800f, 0.800f }, // FCSIM_CCW_WHEEL
	{ 0.000f, 0.000f, 1.000f }, // FCSIM_ROD
	{ 0.420f, 0.204f, 0.000f }, // FCSIM_SOLID_ROD
};

static struct color build_color = { 0.737f, 0.859f, 0.976f };
static struct color goal_color  = { 0.945f, 0.569f, 0.569f };
	
static struct color sky_color = { 0.529f, 0.741f, 0.945f };

static void set_view_wh_from_scale(struct view *view, float scale)
{
	view->w_half = scale * the_width;
	view->h_half = scale * the_height;
}

struct view view;

void vertex2f_world(double x, double y)
{
	coords[cnt_coord++] = (x - view.x) / view.w_half;
	coords[cnt_coord++] = (view.y - y) / view.h_half;
}

static void draw_rect(struct shell *shell)
{
	double sina_half = sin(shell->angle) / 2;
	double cosa_half = cos(shell->angle) / 2;
	double w = fmax(fabs(shell->rect.w), 4.0);
	double h = fmax(fabs(shell->rect.h), 4.0);
	double wc = w * cosa_half;
	double ws = w * sina_half;
	double hc = h * cosa_half;
	double hs = h * sina_half;
	double x = shell->x;
	double y = shell->y;

	indices[cnt_index++] = cnt_coord / 2;
	indices[cnt_index++] = cnt_coord / 2 + 1;
	indices[cnt_index++] = cnt_coord / 2 + 2;
	indices[cnt_index++] = cnt_coord / 2;
	indices[cnt_index++] = cnt_coord / 2 + 2;
	indices[cnt_index++] = cnt_coord / 2 + 3;

	vertex2f_world( wc - hs + x,  ws + hc + y);
	vertex2f_world(-wc - hs + x, -ws + hc + y);
	vertex2f_world(-wc + hs + x, -ws - hc + y);
	vertex2f_world( wc + hs + x,  ws - hc + y);
}

static void draw_area(struct area *area, struct color *color)
{
	double w_half = area->w / 2;
	double h_half = area->h / 2;
	double x = area->x;
	double y = area->y;
	int i;

	for (i = 0; i < 4; i++) {
		colors[cnt_color++] = color->r;
		colors[cnt_color++] = color->g;
		colors[cnt_color++] = color->b;
	}

	indices[cnt_index++] = cnt_coord / 2;
	indices[cnt_index++] = cnt_coord / 2 + 1;
	indices[cnt_index++] = cnt_coord / 2 + 2;
	indices[cnt_index++] = cnt_coord / 2;
	indices[cnt_index++] = cnt_coord / 2 + 2;
	indices[cnt_index++] = cnt_coord / 2 + 3;

	vertex2f_world(x + w_half, y + h_half);
	vertex2f_world(x + w_half, y - h_half);
	vertex2f_world(x - w_half, y - h_half);
	vertex2f_world(x - w_half, y + h_half);
}

#define CIRCLE_SEGMENTS 8

static void draw_circ(struct shell *shell)
{
	double x = shell->x;
	double y = shell->y;
	float r = shell->circ.radius;
	int i;

	for (i = 0; i < CIRCLE_SEGMENTS - 2; i++) {
		indices[cnt_index++] = cnt_coord / 2;
		indices[cnt_index++] = cnt_coord / 2 + i + 1;
		indices[cnt_index++] = cnt_coord / 2 + i + 2;
	}

	for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
		double a = shell->angle + TAU * i / CIRCLE_SEGMENTS;
		vertex2f_world(cos(a) * r + x, sin(a) * r + y);
	}
}

static void draw_block(struct block *block)
{
	struct shell shell;
	int prev_cnt_coord = cnt_coord;

	get_shell(&shell, &block->shape);
	if (block->body) {
		shell.x = block->body->m_position.x;
		shell.y = block->body->m_position.y;
		shell.angle = block->body->m_rotation;
	}

	if (shell.type == SHELL_CIRC)
		draw_circ(&shell);
	else
		draw_rect(&shell);

	for (; prev_cnt_coord < cnt_coord; prev_cnt_coord += 2) {
		colors[cnt_color++] = 0.0;
		colors[cnt_color++] = 0.0;
		colors[cnt_color++] = 0.0;
	}
}

void draw_level(struct arena *arena)
{
	struct design *design = &arena->design;
	struct block *block;

	draw_area(&design->build_area, &build_color);
	draw_area(&design->goal_area, &goal_color);

	for (block = design->level_blocks.head; block; block = block->next)
		draw_block(block);

	for (block = design->player_blocks.head; block; block = block->next)
		draw_block(block);
}

GLuint vertex_shader;
GLuint fragment_shader;
GLuint program;
GLuint coord_buffer;
GLuint color_buffer;
GLuint index_buffer;

GLchar shader_log[1024];

void convert(struct arena *arena)
{
	struct xml_level level;

	xml_parse(galois_xml, sizeof(galois_xml), &level);
	convert_xml(&level, &arena->design);
}

void arena_init(struct arena *arena)
{
	GLint param;
	GLsizei log_len;

	arena->running = 0;
	arena->fast = 0;
	arena->view_scale = 1.0;
	set_view_wh_from_scale(&arena->view, 1.0);

	convert(arena);

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader);
	/*
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &param);
	if (!param) {
		glGetShaderInfoLog(vertex_shader, sizeof(shader_log), &log_len, shader_log);
		printf("vertex shader:\n%s\n", shader_log);
		exit(1);
	}
	*/

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
	glCompileShader(fragment_shader);
	/*
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &param);
	if (!param) {
		glGetShaderInfoLog(fragment_shader, sizeof(shader_log), &log_len, shader_log);
		printf("fragment shader:\n%s", shader_log);
		exit(1);
	}
	*/

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glUseProgram(program);

	glGenBuffers(1, &coord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STREAM_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
}

void arena_draw(struct arena *arena)
{
	glClearColor(0.529f, 0.741f, 0.945f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	cnt_index = 0;
	cnt_coord = 0;
	cnt_color = 0;

	view = arena->view;
	draw_level(arena);

	glBindBuffer(GL_ARRAY_BUFFER, coord_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, cnt_coord * sizeof(coords[0]), coords);

	glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, cnt_color * sizeof(colors[0]), colors);

	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, cnt_index * sizeof(indices[0]), indices);

	glDrawElements(GL_TRIANGLES, cnt_index, GL_UNSIGNED_SHORT, 0);
}

void arena_toggle_fast(struct arena *arena)
{
	arena->fast = !arena->fast;
}

void arena_key_up_event(struct arena *arena, int key)
{

}

void tick_func(void *arg)
{
	struct arena *arena = arg;

	step(arena->world);
}

void arena_key_down_event(struct arena *arena, int key)
{
	switch (key) {
	case 65: /* space */
		if (arena->running) {
			clear_interval(arena->ival);
		} else {
			arena->world = gen_world(&arena->design);
			arena->ival = set_interval(tick_func, 10, arena);
		}
		arena->running = !arena->running;
		break;
	case 39: /* s */
		arena_toggle_fast(arena);
		break;
	}
}

void arena_mouse_move_event(struct arena *arena)
{
	int x = the_cursor_x;
	int y = the_cursor_y;

	int dx_pixel = x - arena->prev_x;
	int dy_pixel = y - arena->prev_y;
	float dx_world = ((float)dx_pixel / the_width) * arena->view.w_half * 2;
	float dy_world = ((float)dy_pixel / the_height) * arena->view.h_half * 2;

	switch (arena->drag.type) {
	case DRAG_PAN:
		arena->view.x -= dx_world;
		arena->view.y -= dy_world;
		break;
	case DRAG_NONE:
		break;
	}

	arena->prev_x = x;
	arena->prev_y = y;
}

void pixel_to_world(struct view *view, int x, int y, float *x_world, float *y_world)
{
	*x_world = view->x + view->w_half * (2 * x - the_width) / the_width;
	*y_world = view->y + view->h_half * (2 * y - the_height) / the_height;
}

static float distance(float x0, float y0, float x1, float y1)
{
	float dx = x1 - x0;
	float dy = y1 - y0;

	return sqrt(dx * dx + dy * dy);
}

void arena_mouse_button_up_event(struct arena *arena, int button)
{
	if (button == 1) /* left */
		arena->drag.type = DRAG_NONE;
}

void arena_mouse_button_down_event(struct arena *arena, int button)
{
	int x = the_cursor_x;
	int y = the_cursor_y;
	float x_world;
	float y_world;
	int vert_id;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	if (arena->running) {
		if (button == 1) // left
			arena->drag.type = DRAG_PAN;
	} else {
		if (button == 1) { // left
			arena->drag.type = DRAG_PAN;
		}
	}
}

void arena_scroll_event(struct arena *arena, int delta)
{
	float scale = 1.0f - delta * 0.05f;

	arena->view_scale *= scale;
	set_view_wh_from_scale(&arena->view, arena->view_scale);
}

void arena_size_event(struct arena *arena)
{
	set_view_wh_from_scale(&arena->view, arena->view_scale);
}
