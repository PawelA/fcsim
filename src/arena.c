#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <box2d/b2Body.h>
#include <box2d/b2World.h>
#include <box2d/b2CMath.h>

#include "gl.h"
#include "xml.h"
#include "interval.h"
#include "graph.h"
#include "arena.h"
#include "galois.h"

#define TAU 6.28318530718

void b2PolyShape_ctor(b2PolyShape *polyShape,
			     const b2ShapeDef* def, b2Body* body,
			     const b2Vec2 newOrigin);

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

	if (block->overlap) {
		color.r = 1.0f;
		color.g = 0.0f;
		color.b = 0.0f;
	} else if (block->goal) {
		color.r = 1.0f;
		color.g = 0.4f;
		color.b = 0.4f;
	} else {
		color.r = block->material->r;
		color.g = block->material->g;
		color.b = block->material->b;
	}

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

	arena->world = gen_world(&arena->design);

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
		arena->world = gen_world(&arena->design);
		clear_interval(arena->ival);
	} else {
		free_world(arena->world, &arena->design);
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
	case 30: /* U */
		arena->tool = TOOL_WHEEL;
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

bool block_has_joint(struct block *block, struct joint *joint)
{
	struct joint *j[5];
	int n;
	int i;

	n = get_block_joints(block, j);

	for (i = 0; i < n; i++) {
		if (j[i] == joint)
			return true;
	}

	return false;
}

bool share_block(struct design *design, struct joint *j1, struct joint *j2)
{
	struct block *block;

	for (block = design->player_blocks.head; block; block = block->next) {
		if (block_has_joint(block, j1) && block_has_joint(block, j2))
			return true;
	}

	return false;
}

struct joint *joint_hit_test_exclude(struct arena *arena, float x, float y, struct rod *rod)
{
	struct design *design = &arena->design;
	struct joint *best_joint = NULL;
	struct joint *joint;
	double best_dist = 8.0f;
	double dist;

	for (joint = design->joints.head; joint; joint = joint->next) {
		if (joint == rod->from)
			continue;
		if (joint == rod->to)
			continue;
		if (share_block(design, rod->from, joint))
			continue;
		dist = distance(x, y, joint->x, joint->y);
		if (dist < best_dist) {
			best_dist = dist;
			best_joint = joint;
		}
	}

	return best_joint;
}

struct joint *joint_hit_test_exclude2(struct arena *arena, float x, float y, struct wheel *wheel)
{
	struct design *design = &arena->design;
	struct joint *best_joint = NULL;
	struct joint *joint;
	double best_dist = 8.0f;
	double dist;

	for (joint = design->joints.head; joint; joint = joint->next) {
		if (joint == wheel->center)
			continue;
		dist = distance(x, y, joint->x, joint->y);
		if (dist < best_dist) {
			best_dist = dist;
			best_joint = joint;
		}
	}

	return best_joint;
}

bool rect_is_hit(struct shell *shell, float x, float y)
{
	float dx = shell->x - x;
	float dy = shell->y - y;
	float s = sinf(shell->angle);
	float c = cosf(shell->angle);
	float dx_t =  dx * c + dy * s;
	float dy_t = -dx * s + dy * c;

	return fabsf(dx_t) < shell->rect.w / 2 && fabsf(dy_t) < shell->rect.h / 2;
}

bool circ_is_hit(struct shell *shell, float x, float y)
{
	float dx = shell->x - x;
	float dy = shell->y - y;
	float r = shell->circ.radius;

	return dx * dx + dy * dy < r * r;
}

bool block_is_hit(struct block *block, float x, float y)
{
	struct shell shell;

	get_shell(&shell, &block->shape);
	if (shell.type == SHELL_CIRC)
		return circ_is_hit(&shell, x, y);
	else
		return rect_is_hit(&shell, x, y);
}

struct block *block_hit_test(struct arena *arena, float x, float y)
{
	struct design *design = &arena->design;
	struct block *block;

	for (block = design->player_blocks.head; block; block = block->next) {
		if (block_is_hit(block, x, y))
			return block;
	}

	return NULL;
}

void gen_block(b2World *world, struct block *block);
void b2World_CleanBodyList(b2World *world);

void mark_overlaps(struct arena *arena)
{
	b2Contact *contact;
	struct block *block;

	b2ContactManager_CleanContactList(&arena->world->m_contactManager);
	b2World_CleanBodyList(arena->world);
	b2ContactManager_Collide(&arena->world->m_contactManager);

	for (block = arena->design.player_blocks.head; block; block = block->next)
		block->overlap = false;

	for (contact = arena->world->m_contactList; contact; contact = contact->m_next) {
		if (contact->m_manifoldCount > 0) {
			block = (struct block *)contact->m_shape1->m_userData;
			block->overlap = true;
			block = (struct block *)contact->m_shape2->m_userData;
			block->overlap = true;
		}
	}

	for (block = arena->design.level_blocks.head; block; block = block->next)
		block->overlap = false;
}

void delete_rod_joints(struct design *design, struct rod *rod)
{
	remove_attach_node(&rod->from->att, rod->from_att);
	free(rod->from_att);
	if (!rod->from->att.head && !rod->from->gen) {
		remove_joint(&design->joints, rod->from);
		free(rod->from);
	}

	remove_attach_node(&rod->to->att, rod->to_att);
	free(rod->to_att);
	if (!rod->to->att.head && !rod->to->gen) {
		remove_joint(&design->joints, rod->to);
		free(rod->to);
	}
}

void delete_wheel_joints(struct design *design, struct wheel *wheel)
{
	int i;

	remove_attach_node(&wheel->center->att, wheel->center_att);
	free(wheel->center_att);
	if (!wheel->center->att.head && !wheel->center->gen) {
		remove_joint(&design->joints, wheel->center);
		free(wheel->center);
	}

	for (i = 0; i < 4; i++) {
		if (wheel->spokes[i]->att.head) {
			wheel->spokes[i]->gen = NULL;
		} else {
			remove_joint(&design->joints, wheel->spokes[i]);
			free(wheel->spokes[i]);
		}
	}
}

void delete_block(struct arena *arena, struct block *block)
{
	struct design *design = &arena->design;
	struct shape *shape = &block->shape;

	switch (shape->type) {
	case SHAPE_ROD:
		delete_rod_joints(design, &shape->rod);
		break;
	case SHAPE_WHEEL:
		delete_wheel_joints(design, &shape->wheel);
	}

	b2World_DestroyBody(arena->world, block->body);
	remove_block(&design->player_blocks, block);
	free(block);

	mark_overlaps(arena);
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

void move_joint(struct arena *arena, struct joint *joint, double x, double y);

void update_wheel_joints(struct arena *arena, struct wheel *wheel)
{
	double a[4] = {
		0.0,
		3.141592653589793 / 2,
		3.141592653589793,
		4.71238898038469,
	};
	double spoke_x, spoke_y;
	double x, y;
	int i;

	x = wheel->center->x;
	y = wheel->center->y;

	for (i = 0; i < 4; i++) {
		spoke_x = x + fp_cos(wheel->angle + a[i]) * wheel->radius;
		spoke_y = y + fp_sin(wheel->angle + a[i]) * wheel->radius;
		move_joint(arena, wheel->spokes[i], spoke_x, spoke_y);
	}
}

void update_joints(struct arena *arena, struct block *block)
{
	struct shape *shape = &block->shape;

	switch (shape->type) {
	case SHAPE_WHEEL:
		update_wheel_joints(arena, &shape->wheel);
		break;
	}
}

void update_body(struct arena *arena, struct block *block)
{
	b2World_DestroyBody(arena->world, block->body);
	gen_block(arena->world, block);
}

void move_joint(struct arena *arena, struct joint *joint, double x, double y)
{
	struct attach_node *node;

	joint->x = x;
	joint->y = y;

	for (node = joint->att.head; node; node = node->next) {
		update_joints(arena, node->block);
		update_body(arena, node->block);
	}
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

	move_joint(arena, joint, joint->x + dx_world, joint->y + dy_world);

	mark_overlaps(arena);
}

bool new_rod_attached(struct rod *rod)
{
	return rod->to->gen || rod->to_att->prev;
}

void attach_new_rod(struct design *design, struct block *block, struct joint *joint)
{
	struct rod *rod = &block->shape.rod;

	remove_joint(&design->joints, rod->to);
	free(rod->to);
	free(rod->to_att);

	rod->to = joint;
	rod->to_att = new_attach_node(block);
	append_attach_node(&joint->att, rod->to_att);
}

void attach_new_wheel(struct arena *arena, struct block *block, struct joint *joint)
{
	struct design *design = &arena->design;
	struct wheel *wheel = &block->shape.wheel;

	remove_joint(&design->joints, wheel->center);
	free(wheel->center);
	free(wheel->center_att);

	wheel->center = joint;
	wheel->center_att = new_attach_node(block);
	append_attach_node(&joint->att, wheel->center_att);

	update_joints(arena, block);
}

void detach_new_rod(struct design *design, struct block *block, double x, double y)
{
	struct rod *rod = &block->shape.rod;

	remove_attach_node(&rod->to->att, rod->to_att);
	free(rod->to_att);

	rod->to = new_joint(NULL, x, y);
	append_joint(&design->joints, rod->to);
	rod->to_att = new_attach_node(block);
	append_attach_node(&rod->to->att, rod->to_att);
}

void detach_new_wheel(struct arena *arena, struct block *block, double x, double y)
{
	struct design *design = &arena->design;
	struct wheel *wheel = &block->shape.wheel;

	remove_attach_node(&wheel->center->att, wheel->center_att);
	free(wheel->center_att);

	wheel->center = new_joint(NULL, x, y);
	append_joint(&design->joints, wheel->center);
	wheel->center_att = new_attach_node(block);
	append_attach_node(&wheel->center->att, wheel->center_att);

	update_joints(arena, block);
}

void adjust_new_rod(struct rod *rod)
{
	double dx = rod->to->x - rod->from->x;
	double dy = rod->to->y - rod->from->y;
	double len = sqrt(dx * dx + dy * dy);

	if (len >= 10.0)
		return;

	if (len == 0.0) {
		dx = 10.0;
		dy = 0.0;
	} else {
		dx *= 10.0 / len;
		dy *= 10.0 / len;
	}
	rod->to->x = rod->from->x + dx;
	rod->to->y = rod->from->y + dy;
}

void action_new_rod(struct arena *arena, int x, int y)
{
	struct rod *rod = &arena->new_block->shape.rod;
	float x_world;
	float y_world;
	struct joint *joint;
	bool attached;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	attached = new_rod_attached(rod);

	if (attached)
		joint = joint_hit_test(arena, x_world, y_world);
	else
		joint = joint_hit_test_exclude(arena, x_world, y_world, rod);

	if (!attached) {
		if (joint) {
			attach_new_rod(&arena->design, arena->new_block, joint);
		} else {
			rod->to->x = x_world;
			rod->to->y = y_world;
			adjust_new_rod(rod);
		}
	} else {
		if (!joint) {
			detach_new_rod(&arena->design, arena->new_block, x_world, y_world);
		} else if (joint != rod->to) {
			detach_new_rod(&arena->design, arena->new_block, x_world, y_world);
			attach_new_rod(&arena->design, arena->new_block, joint);
		}
	}

	update_body(arena, arena->new_block);
	mark_overlaps(arena);

	arena->hover_joint = joint_hit_test(arena, x_world, y_world);
}

void action_new_wheel(struct arena *arena, int x, int y)
{
	struct wheel *wheel = &arena->new_block->shape.wheel;
	float x_world;
	float y_world;
	struct joint *joint;
	bool attached;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	attached = wheel->center->gen || wheel->center_att->prev;

	if (attached)
		joint = joint_hit_test(arena, x_world, y_world);
	else
		joint = joint_hit_test_exclude2(arena, x_world, y_world, wheel);

	if (!attached) {
		if (joint) {
			attach_new_wheel(arena, arena->new_block, joint);
		} else {
			move_joint(arena, wheel->center, x_world, y_world);
		}
	} else {
		if (!joint) {
			detach_new_wheel(arena, arena->new_block, x_world, y_world);
		} else if (joint != wheel->center) {
			detach_new_wheel(arena, arena->new_block, x_world, y_world);
			attach_new_wheel(arena, arena->new_block, joint);
		}
	}

	update_body(arena, arena->new_block);
	mark_overlaps(arena);

	arena->hover_joint = joint_hit_test(arena, x_world, y_world);
}

void action_delete(struct arena *arena, int x, int y)
{
	float x_world;
	float y_world;
	struct block *block;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	block = block_hit_test(arena, x_world, y_world);
	if (block) {
		delete_block(arena, block);
		arena->hover_joint = joint_hit_test(arena, x_world, y_world);
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
	case ACTION_NEW_WHEEL:
		action_new_wheel(arena, x, y);
		break;
	}

	arena->cursor_x = x;
	arena->cursor_y = y;
}

/*
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
	block->overlap = false;

	gen_block(arena->world, block);

	append_block(&design->player_blocks, block);

	arena->hover_joint = j1;

	mark_overlaps(arena);
}
*/

void arena_mouse_button_up_event(struct arena *arena, int button)
{
	if (button != 1) /* left */
		return;

	switch (arena->action) {
	case ACTION_NEW_ROD:
		//new_rod(arena);
		break;
	}

	arena->action = ACTION_NONE;
}

void mouse_down_rod(struct arena *arena, float x, float y)
{
	struct design *design = &arena->design;
	struct block *block;
	struct joint *j0, *j1;
	struct attach_node *att0, *att1;
	bool solid = arena->tool == TOOL_SOLID_ROD;

	block = malloc(sizeof(*block));
	block->prev = NULL;
	block->next = NULL;

	j0 = arena->hover_joint;
	if (!j0) {
		j0 = new_joint(NULL, x, y);
		append_joint(&design->joints, j0);
	}
	att0 = new_attach_node(block);
	append_attach_node(&j0->att, att0);

	j1 = new_joint(NULL, x, y);
	append_joint(&design->joints, j1);
	att1 = new_attach_node(block);
	append_attach_node(&j1->att, att1);

	block->shape.type = SHAPE_ROD;
	block->shape.rod.from = j0;
	block->shape.rod.from_att = att0;
	block->shape.rod.to = j1;
	block->shape.rod.to_att = att1;
	block->shape.rod.width = solid ? 8.0 : 4.0;
	adjust_new_rod(&block->shape.rod);

	block->material = solid ? &solid_rod_material : &water_rod_material;
	block->goal = false;
	block->overlap = false;

	arena->new_block = block;

	gen_block(arena->world, block);

	append_block(&design->player_blocks, block);

	arena->hover_joint = j0;

	arena->action = ACTION_NEW_ROD;
}

void mouse_down_wheel(struct arena *arena, float x, float y)
{
	struct design *design = &arena->design;
	struct block *block;
	struct joint *j0;
	struct attach_node *att0;

	block = malloc(sizeof(*block));
	block->prev = NULL;
	block->next = NULL;

	j0 = arena->hover_joint;
	if (!j0) {
		j0 = new_joint(NULL, x, y);
		append_joint(&design->joints, j0);
	}
	att0 = new_attach_node(block);
	append_attach_node(&j0->att, att0);

	block->shape.type = SHAPE_WHEEL;
	block->shape.wheel.center = j0;
	block->shape.wheel.center_att = att0;
	block->shape.wheel.radius = 20.0;
	block->shape.wheel.angle = 0.0;

	double a[4] = {
		0.0,
		3.141592653589793 / 2,
		3.141592653589793,
		4.71238898038469,
	};
	double spoke_x, spoke_y;
	int i;

	for (i = 0; i < 4; i++) {
		spoke_x = j0->x + fp_cos(a[i]) * 20.0;
		spoke_y = j0->y + fp_sin(a[i]) * 20.0;
		block->shape.wheel.spokes[i] = new_joint(block, spoke_x, spoke_y);
		append_joint(&design->joints, block->shape.wheel.spokes[i]);
	}

	block->material = &solid_material;
	block->goal = false;
	block->overlap = false;

	arena->new_block = block;

	gen_block(arena->world, block);

	append_block(&design->player_blocks, block);

	arena->hover_joint = j0;

	arena->action = ACTION_NEW_WHEEL;
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
			mouse_down_rod(arena, x_world, y_world);
			break;
		case TOOL_WHEEL:
			mouse_down_wheel(arena, x_world, y_world);
			break;
		case TOOL_DELETE:
			action_delete(arena, x, y);
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
