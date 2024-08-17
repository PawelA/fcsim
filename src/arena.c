#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <box2d/b2Body.h>

#include "gl.h"
#include "xml.h"
#include "interval.h"
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

GLuint program;

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

static void push_rect(struct arena *arena, struct shell *shell)
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

	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 1;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 3;

	push_coord(arena,  wc - hs + x,  ws + hc + y);
	push_coord(arena, -wc - hs + x, -ws + hc + y);
	push_coord(arena, -wc + hs + x, -ws - hc + y);
	push_coord(arena,  wc + hs + x,  ws - hc + y);
}

static void push_area(struct arena *arena, struct area *area, struct color *color)
{
	float w_half = area->w / 2;
	float h_half = area->h / 2;
	float x = area->x;
	float y = area->y;
	int i;

	for (i = 0; i < 4; i++) {
		arena->colors[arena->cnt_color++] = color->r;
		arena->colors[arena->cnt_color++] = color->g;
		arena->colors[arena->cnt_color++] = color->b;
	}

	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 1;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 2;
	arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + 3;

	push_coord(arena, x + w_half, y + h_half);
	push_coord(arena, x + w_half, y - h_half);
	push_coord(arena, x - w_half, y - h_half);
	push_coord(arena, x - w_half, y + h_half);
}

#define CIRCLE_SEGMENTS 8

static void push_circ(struct arena *arena, struct shell *shell)
{
	float x = shell->x;
	float y = shell->y;
	float r = shell->circ.radius;
	float a;
	int i;

	for (i = 0; i < CIRCLE_SEGMENTS - 2; i++) {
		arena->indices[arena->cnt_index++] = arena->cnt_coord / 2;
		arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + i + 1;
		arena->indices[arena->cnt_index++] = arena->cnt_coord / 2 + i + 2;
	}

	for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
		a = shell->angle + TAU * i / CIRCLE_SEGMENTS;
		push_coord(arena, cosf(a) * r + x, sinf(a) * r + y);
	}
}

static void push_block(struct arena *arena, struct block *block)
{
	struct shell shell;
	int prev_cnt_coord = arena->cnt_coord;

	get_shell(&shell, &block->shape);
	if (block->body) {
		shell.x = block->body->m_position.x;
		shell.y = block->body->m_position.y;
		shell.angle = block->body->m_rotation;
	}

	if (shell.type == SHELL_CIRC)
		push_circ(arena, &shell);
	else
		push_rect(arena, &shell);

	for (; prev_cnt_coord < arena->cnt_coord; prev_cnt_coord += 2) {
		arena->colors[arena->cnt_color++] = 0.0;
		arena->colors[arena->cnt_color++] = 0.0;
		arena->colors[arena->cnt_color++] = 0.0;
	}
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

void convert(struct arena *arena)
{
	struct xml_level level;

	xml_parse(galois_xml, sizeof(galois_xml), &level);
	convert_xml(&level, &arena->design);
}

void arena_compile_shaders(void)
{
	GLuint vertex_shader;
	GLuint fragment_shader;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
	glCompileShader(fragment_shader);

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

void arena_init(struct arena *arena, float w, float h)
{
	arena->running = false;
	arena->view.x = 0.0f;
	arena->view.y = 0.0f;
	arena->view.width = w;
	arena->view.height = h;
	arena->view.scale = 1.0f;
	arena->cursor_x = 0;
	arena->cursor_y = 0;

	convert(arena);

	glGenBuffers(1, &arena->coord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, arena->coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arena->coords), arena->coords, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &arena->color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, arena->color_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arena->colors), arena->colors, GL_STREAM_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &arena->index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arena->index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(arena->indices), arena->indices, GL_STREAM_DRAW);
}

void arena_draw(struct arena *arena)
{
	glUseProgram(program);

	glClearColor(0.529f, 0.741f, 0.945f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	arena->cnt_index = 0;
	arena->cnt_coord = 0;
	arena->cnt_color = 0;

	draw_level(arena);

	glBindBuffer(GL_ARRAY_BUFFER, arena->coord_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, arena->cnt_coord * sizeof(arena->coords[0]), arena->coords);

	glBindBuffer(GL_ARRAY_BUFFER, arena->color_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, arena->cnt_color * sizeof(arena->colors[0]), arena->colors);

	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, arena->cnt_index * sizeof(arena->indices[0]), arena->indices);

	glDrawElements(GL_TRIANGLES, arena->cnt_index, GL_UNSIGNED_SHORT, 0);
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
	}
}

void arena_mouse_move_event(struct arena *arena, int x, int y)
{
	int dx_pixel = x - arena->cursor_x;
	int dy_pixel = y - arena->cursor_y;
	float dx_world = (float)dx_pixel * arena->view.scale * 2;
	float dy_world = (float)dy_pixel * arena->view.scale * 2;

	switch (arena->action) {
	case ACTION_PAN:
		arena->view.x -= dx_world;
		arena->view.y -= dy_world;
		break;
	case ACTION_NONE:
		break;
	}

	arena->cursor_x = x;
	arena->cursor_y = y;
}

static float distance(float x0, float y0, float x1, float y1)
{
	float dx = x1 - x0;
	float dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
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
