#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <box2d/b2Body.h>

#include "gl.h"
#include "xml.h"
#include "interval.h"
#include "graph.h"
#include "arena.h"
#include "galois.h"

#define TAU 6.28318530718

const char *block_vertex_shader_src =
	"attribute vec2 a_coords;"
	"attribute vec3 a_color;"
	"varying vec3 v_color;"
	"void main() {"
		"gl_Position = vec4(a_coords, 0.0, 1.0);"
		"v_color = a_color;"
	"}";

const char *block_fragment_shader_src =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"#endif\n"
	"varying vec3 v_color;"
	"void main() {"
		"gl_FragColor = vec4(v_color, 1.0);\n"
	"}\n";

GLuint block_program;
GLuint block_program_coord_attrib;
GLuint block_program_color_attrib;

const char *joint_vertex_shader_src =
	"attribute vec2 a_coords;"
	"void main() {"
		"gl_Position = vec4(a_coords, 0.0, 1.0);"
	"}";

const char *joint_fragment_shader_src =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"#endif\n"
	"void main() {"
		"gl_FragColor = vec4(0.5, 0.5, 0.5, 1.0);\n"
	"}\n";

GLuint joint_program;
GLuint joint_program_coord_attrib;

struct color {
	float r;
	float g;
	float b;
};

static struct color build_color = { 0.737f, 0.859f, 0.976f };
static struct color goal_color  = { 0.945f, 0.569f, 0.569f };
static struct color sky_color   = { 0.529f, 0.741f, 0.945f };

void pixel_to_world(struct view *view, int x, int y, float *x_world, float *y_world)
{
	*x_world = view->x + view->scale * (2 * x - view->width);
	*y_world = view->y + view->scale * (2 * y - view->height);
}

void world_to_view(struct view *view, float x, float y, float *x_view, float *y_view)
{
	*x_view = (x - view->x) / (view->width * view->scale);
	*y_view = (view->y - y) / (view->height * view->scale);
}

void push_coord(struct arena *arena, float x, float y)
{
	float x_view;
	float y_view;

	world_to_view(&arena->view, x, y, &x_view, &y_view);

	arena->coords[arena->cnt_coord++] = x_view;
	arena->coords[arena->cnt_coord++] = y_view;
}

void push_color(struct arena *arena, float r, float g, float b)
{
	arena->colors[arena->cnt_color++] = r;
	arena->colors[arena->cnt_color++] = g;
	arena->colors[arena->cnt_color++] = b;
}

void push_indices(struct arena *arena, int n)
{
	int i;

	for (i = 0; i < n - 2; i++) {
		arena->indices[arena->cnt_index++] = arena->cnt_coord / 2;
		arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + i + 1;
		arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + i + 2;
	}
}

static void push_rect(struct arena *arena, struct shell *shell, struct color *color)
{
	float sina_half = sinf(shell->angle) / 2;
	float cosa_half = cosf(shell->angle) / 2;
	float w = fmaxf(fabsf(shell->rect.w), 4.0);
	float h = fmaxf(fabsf(shell->rect.h), 4.0);
	float wc = w * cosa_half;
	float ws = w * sina_half;
	float hc = h * cosa_half;
	float hs = h * sina_half;
	float x = shell->x;
	float y = shell->y;
	int i;

	push_indices(arena, 4);
	push_coord(arena,  wc - hs + x,  ws + hc + y);
	push_coord(arena, -wc - hs + x, -ws + hc + y);
	push_coord(arena, -wc + hs + x, -ws - hc + y);
	push_coord(arena,  wc + hs + x,  ws - hc + y);
	for (i = 0; i < 4; i++)
		push_color(arena, color->r, color->g, color->b);
}

static void push_area(struct arena *arena, struct area *area, struct color *color)
{
	float w_half = area->w / 2;
	float h_half = area->h / 2;
	float x = area->x;
	float y = area->y;
	int i;

	push_indices(arena, 4);
	push_coord(arena, x + w_half, y + h_half);
	push_coord(arena, x + w_half, y - h_half);
	push_coord(arena, x - w_half, y - h_half);
	push_coord(arena, x - w_half, y + h_half);
	for (i = 0; i < 4; i++)
		push_color(arena, color->r, color->g, color->b);
}

#define CIRCLE_SEGMENTS 8

static void push_circ(struct arena *arena, struct shell *shell, struct color *color)
{
	float x = shell->x;
	float y = shell->y;
	float r = shell->circ.radius;
	float a;
	int i;

	push_indices(arena, CIRCLE_SEGMENTS);

	for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
		a = shell->angle + TAU * i / CIRCLE_SEGMENTS;
		push_coord(arena, cosf(a) * r + x, sinf(a) * r + y);
	}

	for (int i = 0; i < CIRCLE_SEGMENTS; i++)
		push_color(arena, color->r, color->g, color->b);
}

static void push_block(struct arena *arena, struct block *block)
{
	struct shell shell;
	struct color color;

	get_shell(&shell, &block->shape);
	if (block->body) {
		shell.x = block->body->m_position.x;
		shell.y = block->body->m_position.y;
		shell.angle = block->body->m_rotation;
	}

	color.r = block->material->r;
	color.g = block->material->g;
	color.b = block->material->b;

	if (shell.type == SHELL_CIRC)
		push_circ(arena, &shell, &color);
	else
		push_rect(arena, &shell, &color);
}

void draw_level(struct arena *arena)
{
	struct design *design = &arena->design;
	struct block *block;

	push_area(arena, &design->build_area, &build_color);
	push_area(arena, &design->goal_area, &goal_color);

	for (block = design->level_blocks.head; block; block = block->next)
		push_block(arena, block);

	for (block = design->player_blocks.head; block; block = block->next)
		push_block(arena, block);
}

bool arena_compile_shaders(void)
{
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLint value;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &block_vertex_shader_src, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &value);
	if (!value)
		return false;

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &block_fragment_shader_src, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &value);
	if (!value)
		return false;

	block_program = glCreateProgram();
	glAttachShader(block_program, vertex_shader);
	glAttachShader(block_program, fragment_shader);
	glLinkProgram(block_program);
	glGetProgramiv(block_program, GL_LINK_STATUS, &value);
	if (!value)
		return false;

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	block_program_coord_attrib = glGetAttribLocation(block_program, "a_coords");
	block_program_color_attrib = glGetAttribLocation(block_program, "a_color");

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &joint_vertex_shader_src, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &value);
	if (!value)
		return false;

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &joint_fragment_shader_src, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &value);
	if (!value)
		return false;

	joint_program = glCreateProgram();
	glAttachShader(joint_program, vertex_shader);
	glAttachShader(joint_program, fragment_shader);
	glLinkProgram(joint_program);
	glGetProgramiv(block_program, GL_LINK_STATUS, &value);
	if (!value)
		return false;

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	joint_program_coord_attrib = glGetAttribLocation(joint_program, "a_coords");

	return true;
}

void arena_init(struct arena *arena, float w, float h)
{
	struct xml_level level;

	arena->running = false;
	arena->view.x = 0.0f;
	arena->view.y = 0.0f;
	arena->view.width = w;
	arena->view.height = h;
	arena->view.scale = 1.0f;
	arena->cursor_x = 0;
	arena->cursor_y = 0;

	arena->action = ACTION_NONE;
	arena->hover_joint = NULL;

	xml_parse(galois_xml, sizeof(galois_xml), &level);
	convert_xml(&level, &arena->design);

	glGenBuffers(1, &arena->coord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, arena->coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arena->coords), arena->coords, GL_STREAM_DRAW);

	glGenBuffers(1, &arena->color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, arena->color_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arena->colors), arena->colors, GL_STREAM_DRAW);

	glGenBuffers(1, &arena->index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arena->index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(arena->indices), arena->indices, GL_STREAM_DRAW);

	glGenBuffers(1, &arena->joint_coord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, arena->joint_coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arena->joint_coords), arena->joint_coords, GL_STREAM_DRAW);
}

void fill_joint_coords(struct arena *arena, struct joint *joint)
{
	float x, y;
	float a0, x0, y0;
	float a1, x1, y1;
	float r = 0.02f / arena->view.scale;
	int c = 0;
	int i;

	world_to_view(&arena->view, joint->x, joint->y, &x, &y);

	for (i = 0; i < 8; i++) {
		a0 = i * (TAU / 8);
		x0 = x + cosf(a0) * r;
		y0 = y + sinf(a0) * r;
		a1 = (i + 1) * (TAU / 8);
		x1 = x + cosf(a1) * r;
		y1 = y + sinf(a1) * r;
		arena->joint_coords[c++] = x0;
		arena->joint_coords[c++] = y0;
		arena->joint_coords[c++] = x;
		arena->joint_coords[c++] = y;
		arena->joint_coords[c++] = x1;
		arena->joint_coords[c++] = y1;
	}
}

void arena_draw(struct arena *arena)
{
	glClearColor(sky_color.r, sky_color.g, sky_color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	arena->cnt_index = 0;
	arena->cnt_coord = 0;
	arena->cnt_color = 0;

	draw_level(arena);

	glUseProgram(block_program);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, arena->coord_buffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBufferSubData(GL_ARRAY_BUFFER, 0, arena->cnt_coord * sizeof(arena->coords[0]), arena->coords);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, arena->color_buffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBufferSubData(GL_ARRAY_BUFFER, 0, arena->cnt_color * sizeof(arena->colors[0]), arena->colors);

	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, arena->cnt_index * sizeof(arena->indices[0]), arena->indices);

	glDrawElements(GL_TRIANGLES, arena->cnt_index, GL_UNSIGNED_SHORT, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	if (arena->hover_joint) {
		fill_joint_coords(arena, arena->hover_joint);

		glUseProgram(joint_program);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, arena->joint_coord_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBufferSubData(GL_ARRAY_BUFFER, 0, 48 * sizeof(float), arena->joint_coords);
		glDrawArrays(GL_TRIANGLES, 0, 24);
		glDisableVertexAttribArray(0);
	}
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
			free_world(arena->world, &arena->design);
			arena->world = NULL;
			clear_interval(arena->ival);
		} else {
			arena->world = gen_world(&arena->design);
			arena->ival = set_interval(tick_func, 10, arena);
			arena->hover_joint = NULL;
		}
		arena->running = !arena->running;
		break;
	}
}

static float distance(float x0, float y0, float x1, float y1)
{
	float dx = x1 - x0;
	float dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

struct joint *joint_hit_test(struct arena *arena, float x, float y)
{
	struct design *design = &arena->design;
	struct joint *joint;

	for (joint = design->joints.head; joint; joint = joint->next) {
		if (distance(x, y, joint->x, joint->y) < 10.0f)
			return joint;
	}

	return NULL;
}

void action_pan(struct arena *arena, int x, int y)
{
	int dx_pixel = x - arena->cursor_x;
	int dy_pixel = y - arena->cursor_y;
	float dx_world = (float)dx_pixel * arena->view.scale * 2;
	float dy_world = (float)dy_pixel * arena->view.scale * 2;

	arena->view.x -= dx_world;
	arena->view.y -= dy_world;
}

void action_none(struct arena *arena, int x, int y)
{
	float x_world;
	float y_world;
	struct joint *joint;

	if (arena->running)
		return;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	joint = joint_hit_test(arena, x_world, y_world);
	arena->hover_joint = joint;
}

void arena_mouse_move_event(struct arena *arena, int x, int y)
{
	switch (arena->action) {
	case ACTION_PAN:
		action_pan(arena, x, y);
		break;
	case ACTION_NONE:
		action_none(arena, x, y);
		break;
	}

	arena->cursor_x = x;
	arena->cursor_y = y;
}

void arena_mouse_button_up_event(struct arena *arena, int button)
{
	if (button == 1) /* left */
		arena->action = ACTION_NONE;
}

void arena_mouse_button_down_event(struct arena *arena, int button)
{
	int x = arena->cursor_x;
	int y = arena->cursor_y;
	float x_world;
	float y_world;

	if (button != 1) // left
		return;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	if (arena->running) {
		arena->action = ACTION_PAN;
	} else {
		arena->action = ACTION_PAN;
	}
}

void arena_scroll_event(struct arena *arena, int delta)
{
	arena->view.scale *= (1.0f - delta * 0.05f);
}

void arena_size_event(struct arena *arena, float width, float height)
{
	arena->view.width = width;
	arena->view.height = height;
}
