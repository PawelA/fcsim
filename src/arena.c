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

	arena->tool = TOOL_MOVE;
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
	float x = joint->x;
	float y = joint->y;
	float a0, x0, y0;
	float a1, x1, y1;
	float r = 4.0f;
	float *coords = arena->joint_coords;
	int i;

	for (i = 0; i < 8; i++) {
		a0 = i * (TAU / 8);
		a1 = (i + 1) * (TAU / 8);
		x0 = x + cosf(a0) * r;
		y0 = y + sinf(a0) * r;
		x1 = x + cosf(a1) * r;
		y1 = y + sinf(a1) * r;
		world_to_view(&arena->view, x,  y,  coords + 2, coords + 3);
		world_to_view(&arena->view, x0, y0, coords + 0, coords + 1);
		world_to_view(&arena->view, x1, y1, coords + 4, coords + 5);
		coords += 6;
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

void start_stop(struct arena *arena)
{
	if (arena->running) {
		free_world(arena->world, &arena->design);
		arena->world = NULL;
		clear_interval(arena->ival);
	} else {
		arena->world = gen_world(&arena->design);
		arena->ival = set_interval(tick_func, 10, arena);
		arena->hover_joint = NULL;
		arena->action = ACTION_NONE;
	}
	arena->running = !arena->running;
}

void arena_key_down_event(struct arena *arena, int key)
{
	switch (key) {
	case 65: /* space */
		start_stop(arena);
		break;
	case 27: /* R */
		arena->tool = TOOL_ROD;
		break;
	case 58: /* M */
		arena->tool = TOOL_MOVE;
		break;
	case 39: /* S */
		arena->tool = TOOL_SOLID_ROD;
		break;
	case 40: /* D */
		arena->tool = TOOL_DELETE;
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
	struct joint *best_joint = NULL;
	struct joint *joint;
	double best_dist = 8.0f;
	double dist;

	for (joint = design->joints.head; joint; joint = joint->next) {
		dist = distance(x, y, joint->x, joint->y);
		if (dist < best_dist) {
			best_dist = dist;
			best_joint = joint;
		}
	}

	return best_joint;
}

void action_pan(struct arena *arena, int x, int y)
{
	float dx_world;
	float dy_world;

	dx_world = (x - arena->cursor_x) * arena->view.scale * 2;
	dy_world = (y - arena->cursor_y) * arena->view.scale * 2;

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

void action_move_joint(struct arena *arena, int x, int y)
{
	float dx_world;
	float dy_world;
	struct joint *joint = arena->hover_joint;

	if (joint->gen)
		return;

	dx_world = (x - arena->cursor_x) * arena->view.scale * 2;
	dy_world = (y - arena->cursor_y) * arena->view.scale * 2;

	joint->x += dx_world;
	joint->y += dy_world;
}

void action_new_rod(struct arena *arena, int x, int y)
{
	float x_world;
	float y_world;
	struct joint *joint;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	joint = joint_hit_test(arena, x_world, y_world);
	arena->hover_joint = joint;

	if (joint && joint != arena->new_rod.j0) {
		arena->new_rod.j1 = joint;
		arena->new_rod.x1 = joint->x;
		arena->new_rod.y1 = joint->y;
	} else {
		arena->new_rod.j1 = NULL;
		arena->new_rod.x1 = x_world;
		arena->new_rod.y1 = y_world;
	}
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
	case ACTION_MOVE_JOINT:
		action_move_joint(arena, x, y);
		break;
	case ACTION_NEW_ROD:
		action_new_rod(arena, x, y);
		break;
	}

	arena->cursor_x = x;
	arena->cursor_y = y;
}

void new_rod(struct arena *arena)
{
	struct new_rod *new_rod = &arena->new_rod;
	struct design *design = &arena->design;
	struct block *block;
	struct joint *j0, *j1;
	struct attach_node *att0, *att1;

	block = malloc(sizeof(*block));
	block->prev = NULL;
	block->next = NULL;

	j0 = new_rod->j0;
	if (!j0) {
		j0 = new_joint(NULL, new_rod->x0, new_rod->y0);
		append_joint(&design->joints, j0);
	}
	att0 = new_attach_node(block);
	append_attach_node(&j0->att, att0);

	j1 = new_rod->j1;
	if (!j1) {
		j1 = new_joint(NULL, new_rod->x1, new_rod->y1);
		append_joint(&design->joints, j1);
	}
	att1 = new_attach_node(block);
	append_attach_node(&j1->att, att1);

	block->shape.type = SHAPE_ROD;
	block->shape.rod.from = j0;
	block->shape.rod.from_att = att0;
	block->shape.rod.to = j1;
	block->shape.rod.to_att = att1;
	block->shape.rod.width = new_rod->solid ? 8.0 : 4.0;

	block->material = new_rod->solid ? &solid_rod_material : &water_rod_material;
	block->goal = false;
	block->body = NULL;

	append_block(&design->player_blocks, block);

	arena->hover_joint = j1; /* HACK! */
}

void arena_mouse_button_up_event(struct arena *arena, int button)
{
	if (button != 1) /* left */
		return;

	switch (arena->action) {
	case ACTION_NEW_ROD:
		new_rod(arena);
		break;
	}

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
		switch (arena->tool) {
		case TOOL_MOVE:
			if (arena->hover_joint)
				arena->action = ACTION_MOVE_JOINT;
			else
				arena->action = ACTION_PAN;
			break;
		case TOOL_ROD:
		case TOOL_SOLID_ROD:
			arena->action = ACTION_NEW_ROD;
			if (arena->hover_joint) {
				arena->new_rod.j0 = arena->hover_joint;
				arena->new_rod.x0 = arena->hover_joint->x;
				arena->new_rod.y0 = arena->hover_joint->y;
			} else {
				arena->new_rod.j0 = NULL;
				arena->new_rod.x0 = x_world;
				arena->new_rod.y0 = y_world;
			}
			arena->new_rod.j1 = NULL;
			arena->new_rod.x1 = x_world;
			arena->new_rod.y1 = y_world;
			arena->new_rod.solid = (arena->tool == TOOL_SOLID_ROD);
			break;
		}
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
