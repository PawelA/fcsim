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
#include "poocs.h"

#define TAU 6.28318530718

void b2PolyShape_ctor(b2PolyShape *polyShape,
			     const b2ShapeDef* def, b2Body* body,
			     const b2Vec2 newOrigin);

const char *block_vertex_shader_src =
	"attribute vec2 a_coords;"
	"attribute vec3 a_color;"
	"varying vec3 v_color;"
	"uniform vec2 u_scale;"
	"uniform vec2 u_shift;"
	"void main() {"
		"gl_Position = vec4(a_coords * u_scale + u_shift, 0.0, 1.0);"
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
GLuint block_program_scale_uniform;
GLuint block_program_shift_uniform;

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

static void block_graphics_add_area(struct block_graphics *graphics,
				    struct area *area, struct color *color)
{
	float w_half = area->w / 2;
	float h_half = area->h / 2;
	int i;

	unsigned short *indices = graphics->indices + (graphics->triangle_cnt * 3);
	float *coords = graphics->coords + (graphics->vertex_cnt * 2);
	float *colors = graphics->colors + (graphics->vertex_cnt * 3);

	for (i = 0; i < 2; i++) {
		*indices++ = graphics->vertex_cnt;
		*indices++ = graphics->vertex_cnt + i + 1;
		*indices++ = graphics->vertex_cnt + i + 2;
	}

	*coords++ = area->x + w_half;
	*coords++ = area->y + h_half;
	*coords++ = area->x + w_half;
	*coords++ = area->y - h_half;
	*coords++ = area->x - w_half;
	*coords++ = area->y - h_half;
	*coords++ = area->x - w_half;
	*coords++ = area->y + h_half;

	for (i = 0; i < 4; i++) {
		*colors++ = color->r;
		*colors++ = color->g;
		*colors++ = color->b;
	}

	graphics->triangle_cnt += 2;
	graphics->vertex_cnt += 4;
}

static void block_graphics_add_rect(struct block_graphics *graphics,
				    struct shell *shell, struct color *color)
{
	float sina_half = sinf(shell->angle) / 2;
	float cosa_half = cosf(shell->angle) / 2;
	float w = fmaxf(fabsf(shell->rect.w), 4.0);
	float h = fmaxf(fabsf(shell->rect.h), 4.0);
	float wc = w * cosa_half;
	float ws = w * sina_half;
	float hc = h * cosa_half;
	float hs = h * sina_half;
	int i;

	unsigned short *indices = graphics->indices + (graphics->triangle_cnt * 3);
	float *coords = graphics->coords + (graphics->vertex_cnt * 2);
	float *colors = graphics->colors + (graphics->vertex_cnt * 3);

	for (i = 0; i < 2; i++) {
		*indices++ = graphics->vertex_cnt;
		*indices++ = graphics->vertex_cnt + i + 1;
		*indices++ = graphics->vertex_cnt + i + 2;
	}

	*coords++ = shell->x + wc - hs;
	*coords++ = shell->y + ws + hc;
	*coords++ = shell->x - wc - hs;
	*coords++ = shell->y - ws + hc;
	*coords++ = shell->x - wc + hs;
	*coords++ = shell->y - ws - hc;
	*coords++ = shell->x + wc + hs;
	*coords++ = shell->y + ws - hc;

	for (i = 0; i < 4; i++) {
		*colors++ = color->r;
		*colors++ = color->g;
		*colors++ = color->b;
	}

	graphics->triangle_cnt += 2;
	graphics->vertex_cnt += 4;
}

#define CIRCLE_SEGMENTS 8

static void block_graphics_add_circ(struct block_graphics *graphics,
				    struct shell *shell, struct color *color)
{
	unsigned short *indices = graphics->indices + (graphics->triangle_cnt * 3);
	float *coords = graphics->coords + (graphics->vertex_cnt * 2);
	float *colors = graphics->colors + (graphics->vertex_cnt * 3);
	float a;
	int i;

	for (i = 0; i < CIRCLE_SEGMENTS - 2; i++) {
		*indices++ = graphics->vertex_cnt;
		*indices++ = graphics->vertex_cnt + i + 1;
		*indices++ = graphics->vertex_cnt + i + 2;
	}

	for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
		a = shell->angle + TAU * i / CIRCLE_SEGMENTS;
		*coords++ = shell->x + cosf(a) * shell->circ.radius;
		*coords++ = shell->y + sinf(a) * shell->circ.radius;
	}

	for (i = 0; i < CIRCLE_SEGMENTS; i++) {
		*colors++ = color->r;
		*colors++ = color->g;
		*colors++ = color->b;
	}

	graphics->triangle_cnt += CIRCLE_SEGMENTS - 2;
	graphics->vertex_cnt += CIRCLE_SEGMENTS;
}

static void block_graphics_add_block(struct block_graphics *graphics,
				     struct block *block)
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
	} else if (block->visited) {
		color.r = block->r + (1.0f - block->r) * 0.25f;
		color.g = block->g + (1.0f - block->g) * 0.25f;
		color.b = block->b + (1.0f - block->b) * 0.25f;
	} else {
		color.r = block->r;
		color.g = block->g;
		color.b = block->b;
	}

	if (shell.type == SHELL_CIRC)
		block_graphics_add_circ(graphics, &shell, &color);
	else
		block_graphics_add_rect(graphics, &shell, &color);
}

void add_block_mesh_cnts(struct block *block, int *vertex_cnt, int *triangle_cnt)
{
	enum shape_type type = block->shape.type;
	int v;

	if (type == SHAPE_CIRC || type == SHAPE_WHEEL)
		v = CIRCLE_SEGMENTS;
	else
		v = 4;

	*vertex_cnt += v;
	*triangle_cnt += v - 2;
}

void get_mesh_cnts(struct design *design, int *vertex_cnt, int *triangle_cnt)
{
	struct block *block;

	*vertex_cnt = 8;
	*triangle_cnt = 4;

	for (block = design->level_blocks.head; block; block = block->next)
		add_block_mesh_cnts(block, vertex_cnt, triangle_cnt);

	for (block = design->player_blocks.head; block; block = block->next)
		add_block_mesh_cnts(block, vertex_cnt, triangle_cnt);
}

void block_graphics_reset(struct block_graphics *graphics, struct design *design)
{
	struct block *block;
	int vertex_cnt;
	int triangle_cnt;
	int indices_size;
	int coords_size;
	int colors_size;

	get_mesh_cnts(design, &vertex_cnt, &triangle_cnt);

	indices_size = triangle_cnt * 3 * sizeof(unsigned short);
	coords_size = vertex_cnt * 2 * sizeof(float);
	colors_size = vertex_cnt * 3 * sizeof(float);

	free(graphics->indices);
	free(graphics->coords);
	free(graphics->colors);

	graphics->indices = malloc(indices_size);
	graphics->coords = malloc(coords_size);
	graphics->colors = malloc(colors_size);

	graphics->vertex_cnt = 0;
	graphics->triangle_cnt = 0;

	block_graphics_add_area(graphics, &design->build_area, &build_color);
	block_graphics_add_area(graphics, &design->goal_area, &goal_color);

	for (block = design->level_blocks.head; block; block = block->next)
		block_graphics_add_block(graphics, block);

	for (block = design->player_blocks.head; block; block = block->next)
		block_graphics_add_block(graphics, block);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, graphics->index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, graphics->indices,
		     GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, graphics->coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, coords_size, graphics->coords, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, graphics->color_buffer);
	glBufferData(GL_ARRAY_BUFFER, colors_size, graphics->colors, GL_STATIC_DRAW);
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
	block_program_scale_uniform = glGetUniformLocation(block_program, "u_scale");
	block_program_shift_uniform = glGetUniformLocation(block_program, "u_shift");

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

void block_graphics_init(struct block_graphics *graphics)
{
	graphics->indices = NULL;
	graphics->coords = NULL;
	graphics->colors = NULL;

	glGenBuffers(1, &graphics->index_buffer);
	glGenBuffers(1, &graphics->coord_buffer);
	glGenBuffers(1, &graphics->color_buffer);

	graphics->triangle_cnt = 0;
	graphics->vertex_cnt = 0;
}

void arena_init(struct arena *arena, float w, float h)
{
	struct xml_level level;

	arena->view.x = 0.0f;
	arena->view.y = 0.0f;
	arena->view.width = w;
	arena->view.height = h;
	arena->view.scale = 1.0f;
	arena->cursor_x = 0;
	arena->cursor_y = 0;

	arena->shift = false;
	arena->ctrl = false;

	arena->tool = TOOL_MOVE;
	arena->tool_hidden = TOOL_MOVE;
	arena->state = STATE_NORMAL;
	arena->hover_joint = NULL;
	arena->hover_block = NULL;

	arena->root_joints_moving = NULL;
	arena->root_blocks_moving = NULL;
	arena->blocks_moving = NULL;

	xml_parse(poocs_xml, sizeof(poocs_xml), &level);
	convert_xml(&level, &arena->design);

	arena->world = gen_world(&arena->design);

	block_graphics_init(&arena->block_graphics);

	glGenBuffers(1, &arena->joint_coord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, arena->joint_coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arena->joint_coords),
		     arena->joint_coords, GL_STREAM_DRAW);
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

void block_graphics_draw(struct block_graphics *graphics, struct view *view)
{
	glUseProgram(block_program);

	glUniform2f(block_program_scale_uniform,
		     1.0f / (view->width * view->scale),
		    -1.0f / (view->height * view->scale));
	glUniform2f(block_program_shift_uniform,
		    -view->x / (view->width * view->scale),
		     view->y / (view->height * view->scale));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, graphics->index_buffer);

	glBindBuffer(GL_ARRAY_BUFFER, graphics->coord_buffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, graphics->color_buffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawElements(GL_TRIANGLES, graphics->triangle_cnt * 3, GL_UNSIGNED_SHORT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void arena_draw(struct arena *arena)
{
	glClearColor(sky_color.r, sky_color.g, sky_color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	block_graphics_reset(&arena->block_graphics, &arena->design);
	block_graphics_draw(&arena->block_graphics, &arena->view);

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

void update_tool(struct arena *arena)
{
	if (arena->shift)
		arena->tool = TOOL_MOVE;
	else if (arena->ctrl)
		arena->tool = TOOL_DELETE;
	else
		arena->tool = arena->tool_hidden;
}

void arena_key_up_event(struct arena *arena, int key)
{
	switch (key) {
	case 50: /* shift */
		arena->shift = false;
		break;
	case 37: /* ctrl */
		arena->ctrl = false;
		break;
	}

	update_tool(arena);
}

void tick_func(void *arg)
{
	struct arena *arena = arg;

	step(arena->world);
}

void start_stop(struct arena *arena)
{
	if (arena->state == STATE_RUNNING || arena->state == STATE_RUNNING_PAN) {
		free_world(arena->world, &arena->design);
		arena->world = gen_world(&arena->design);
		clear_interval(arena->ival);
		arena->state = arena->state == STATE_RUNNING ?
			       STATE_NORMAL :
			       STATE_NORMAL_PAN;
	} else {
		free_world(arena->world, &arena->design);
		arena->world = gen_world(&arena->design);
		arena->ival = set_interval(tick_func, 10, arena);
		arena->hover_joint = NULL;
		arena->state = STATE_RUNNING;
	}
}

void arena_key_down_event(struct arena *arena, int key)
{
	switch (key) {
	case 65: /* space */
		start_stop(arena);
		break;
	case 27: /* R */
		arena->tool_hidden = TOOL_ROD;
		break;
	case 58: /* M */
		arena->tool_hidden = TOOL_MOVE;
		break;
	case 39: /* S */
		arena->tool_hidden = TOOL_SOLID_ROD;
		break;
	case 40: /* D */
		arena->tool_hidden = TOOL_DELETE;
		break;
	case 30: /* U */
		arena->tool_hidden = TOOL_WHEEL;
		break;
	case 25: /* W */
		arena->tool_hidden = TOOL_CW_WHEEL;
		break;
	case 54: /* C */
		arena->tool_hidden = TOOL_CCW_WHEEL;
		break;
	case 50: /* shift */
		arena->shift = true;
		break;
	case 37: /* ctrl */
		arena->ctrl = true;
		break;
	}

	update_tool(arena);
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

struct joint *joint_hit_test_exclude_rod(struct arena *arena, float x, float y, struct rod *rod, bool attached)
{
	struct design *design = &arena->design;
	struct joint *best_joint = NULL;
	struct joint *joint;
	double best_dist = 8.0f;
	double dist;

	for (joint = design->joints.head; joint; joint = joint->next) {
		if (joint == rod->from)
			continue;
		if (!attached && joint == rod->to)
			continue;
		if (!attached && share_block(design, rod->from, joint))
			continue;
		dist = distance(x, y, joint->x, joint->y);
		if (dist < best_dist) {
			best_dist = dist;
			best_joint = joint;
		}
	}

	return best_joint;
}

bool has_wheel(struct joint *joint, struct wheel *not_this_one)
{
	struct attach_node *node;

	if (joint->gen && joint->gen->shape.type == SHAPE_WHEEL)
		return true;

	for (node = joint->att.head; node; node = node->next) {
		if (node->block->shape.type == SHAPE_WHEEL &&
		    &node->block->shape.wheel != not_this_one)
			return true;
	}

	return false;
}

struct joint *joint_hit_test_exclude_wheel(struct arena *arena, float x, float y, struct wheel *wheel, bool attached)
{
	struct design *design = &arena->design;
	struct joint *best_joint = NULL;
	struct joint *joint;
	double best_dist = 8.0f;
	double dist;

	for (joint = design->joints.head; joint; joint = joint->next) {
		if (!attached && joint == wheel->center)
			continue;
		if (joint == wheel->spokes[0])
			continue;
		if (joint == wheel->spokes[1])
			continue;
		if (joint == wheel->spokes[2])
			continue;
		if (joint == wheel->spokes[3])
			continue;
		if (has_wheel(joint, wheel))
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

	shell->rect.w += 8.0 * 2;
	shell->rect.h += 8.0 * 2;

	return fabsf(dx_t) < shell->rect.w / 2 && fabsf(dy_t) < shell->rect.h / 2;
}

bool circ_is_hit(struct shell *shell, float x, float y)
{
	float dx = shell->x - x;
	float dy = shell->y - y;
	float r = shell->circ.radius + 8.0;

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

static void get_rect_bb(struct shell *shell, struct area *area)
{
	float sina = fp_sin(shell->angle);
	float cosa = fp_cos(shell->angle);
	float wc = shell->rect.w * cosa;
	float ws = shell->rect.w * sina;
	float hc = shell->rect.h * cosa;
	float hs = shell->rect.h * sina;

	area->x = shell->x;
	area->y = shell->y;
	area->w = fabs(wc) + fabs(hs);
	area->h = fabs(ws) + fabs(hc);
}

static void get_circ_bb(struct shell *shell, struct area *area)
{
	area->x = shell->x;
	area->y = shell->y;
	area->w = shell->circ.radius * 2;
	area->h = shell->circ.radius * 2;
}

static void get_block_bb(struct block *block, struct area *area)
{
	struct shell shell;

	get_shell(&shell, &block->shape);

	if (shell.type == SHELL_CIRC)
		get_circ_bb(&shell, area);
	else
		get_rect_bb(&shell, area);
}

static int block_inside_area(struct block *block, struct area *area)
{
	struct area bb;

	get_block_bb(block, &bb);

	return bb.x - bb.w / 2 >= area->x - area->w / 2
	    && bb.x + bb.w / 2 <= area->x + area->w / 2
	    && bb.y - bb.h / 2 >= area->y - area->h / 2
	    && bb.y + bb.h / 2 <= area->y + area->h / 2;
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

	for (block = arena->design.player_blocks.head; block; block = block->next) {
		block->overlap = false;
		if (!block->goal && !block_inside_area(block, &arena->design.build_area))
			block->overlap = true;
	}

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
	struct block *block;

	if (arena->state == STATE_RUNNING || arena->state == STATE_RUNNING_PAN)
		return;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	joint = joint_hit_test(arena, x_world, y_world);
	if (joint) {
		arena->hover_joint = joint;
		arena->hover_block = NULL;
		return;
	}

	block = block_hit_test(arena, x_world, y_world);
	if (block) {
		arena->hover_joint = NULL;
		arena->hover_block = block;
		return;
	}

	arena->hover_joint = NULL;
	arena->hover_block = NULL;
}

void move_joint(struct arena *arena, struct joint *joint, double x, double y);

void update_wheel_joints2(struct wheel *wheel)
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
		wheel->spokes[i]->x = x + fp_cos(wheel->angle + a[i]) * wheel->radius;
		wheel->spokes[i]->y = y + fp_sin(wheel->angle + a[i]) * wheel->radius;
	}
}

void update_joints2(struct block *block)
{
	struct shape *shape = &block->shape;

	switch (shape->type) {
	case SHAPE_WHEEL:
		update_wheel_joints2(&shape->wheel);
		break;
	}
}

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

void update_move(struct arena *arena, double dx, double dy)
{
	struct joint_head *joint_head;
	struct block_head *block_head;

	for (joint_head = arena->root_joints_moving; joint_head;
	     joint_head = joint_head->next) {
		joint_head->joint->x = joint_head->orig_x + dx;
		joint_head->joint->y = joint_head->orig_y + dy;
	}

	for (block_head = arena->blocks_moving; block_head;
	     block_head = block_head->next) {
		update_joints2(block_head->block);
	}

	for (block_head = arena->blocks_moving; block_head;
	     block_head = block_head->next) {
		update_body(arena, block_head->block);
	}

	mark_overlaps(arena);
}

void action_move(struct arena *arena, int x, int y)
{
	float x_world;
	float y_world;
	float dx;
	float dy;

	pixel_to_world(&arena->view, x, y, &x_world, &y_world);

	dx = x_world - arena->move_orig_x;
	dy = y_world - arena->move_orig_y;

	update_move(arena, dx, dy);
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

	joint = joint_hit_test_exclude_rod(arena, x_world, y_world, rod, attached);

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

	joint = joint_hit_test_exclude_wheel(arena, x_world, y_world, wheel, attached);

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

void mouse_down_delete(struct arena *arena, float x, float y)
{
	struct block *block;

	block = block_hit_test(arena, x, y);
	if (block) {
		delete_block(arena, block);
		arena->hover_joint = joint_hit_test(arena, x, y);
	}
}

void arena_mouse_move_event(struct arena *arena, int x, int y)
{
	switch (arena->state) {
	case STATE_NORMAL_PAN:
	case STATE_RUNNING_PAN:
		action_pan(arena, x, y);
		break;
	case STATE_NORMAL:
	case STATE_RUNNING:
		action_none(arena, x, y);
		break;
	case STATE_MOVE:
		action_move(arena, x, y);
		break;
	case STATE_NEW_ROD:
		action_new_rod(arena, x, y);
		break;
	case STATE_NEW_WHEEL:
		action_new_wheel(arena, x, y);
		break;
	}

	arena->cursor_x = x;
	arena->cursor_y = y;
}

void joint_dfs(struct arena *arena, struct joint *joint, bool value, bool all);
void block_dfs(struct arena *arena, struct block *block, bool value, bool all);

void mouse_up_move(struct arena *arena)
{
	struct block_head *block_head;
	bool overlap = false;

	if (arena->move_orig_joint)
		joint_dfs(arena, arena->move_orig_joint, false, true);
	else
		block_dfs(arena, arena->move_orig_block, false, true);

	arena->move_orig_joint = NULL;
	arena->move_orig_block = NULL;

	for (block_head = arena->blocks_moving; block_head;
	     block_head = block_head->next) {
		if (block_head->block->overlap) {
			overlap = true;
			break;
		}
	}

	if (overlap)
		update_move(arena, 0.0, 0.0);

	arena->root_joints_moving = NULL;
	arena->root_blocks_moving = NULL;
	arena->blocks_moving = NULL;
}

void mouse_up_new_block(struct arena *arena)
{
	if (arena->new_block->overlap) {
		delete_block(arena, arena->new_block);
		arena->hover_joint = NULL;
	}
}

void arena_mouse_button_up_event(struct arena *arena, int button)
{
	if (button != 1) /* left */
		return;

	switch (arena->state) {
	case STATE_NORMAL_PAN:
		arena->state = STATE_NORMAL;
		break;
	case STATE_MOVE:
		mouse_up_move(arena);
		arena->state = STATE_NORMAL;
		break;
	case STATE_NEW_ROD:
	case STATE_NEW_WHEEL:
		mouse_up_new_block(arena);
		arena->state = STATE_NORMAL;
		break;
	case STATE_RUNNING_PAN:
		arena->state = STATE_RUNNING;
		break;
	}

}

struct joint_head *append_joint_head(struct joint_head *head, struct joint *joint)
{
	struct joint_head *new_head;

	new_head = malloc(sizeof(*new_head));
	new_head->next = head;
	new_head->joint = joint;
	new_head->orig_x = joint->x;
	new_head->orig_y = joint->y;

	return new_head;
}

struct block_head *append_block_head(struct block_head *head, struct block *block)
{
	struct block_head *new_head;

	new_head = malloc(sizeof(*new_head));
	new_head->next = head;
	new_head->block = block;

	return new_head;
}

void block_dfs(struct arena *arena, struct block *block, bool value, bool all)
{
	struct shape *shape = &block->shape;
	int i;

	if (block->visited == value)
		return;
	block->visited = value;

	arena->blocks_moving = append_block_head(arena->blocks_moving, block);

	switch (shape->type) {
	case SHAPE_BOX:
		joint_dfs(arena, shape->box.center, value, all);
		for (i = 0; i < 4; i++)
			joint_dfs(arena, shape->box.corners[i], value, all);
		break;
	case SHAPE_ROD:
		if (all) {
			joint_dfs(arena, shape->rod.from, value, all);
			joint_dfs(arena, shape->rod.to, value, all);
		}
		break;
	case SHAPE_WHEEL:
		if (all)
			joint_dfs(arena, shape->wheel.center, value, all);
		for (i = 0; i < 4; i++)
			joint_dfs(arena, shape->wheel.spokes[i], value, all);
		break;
	}
}

void joint_dfs(struct arena *arena, struct joint *joint, bool value, bool all)
{
	struct attach_node *node;

	if (joint->visited == value)
		return;
	joint->visited = value;

	if (!joint->gen) {
		arena->root_joints_moving =
			append_joint_head(arena->root_joints_moving, joint);
	}

	if (all && joint->gen)
		block_dfs(arena, joint->gen, value, all);

	for (node = joint->att.head; node; node = node->next)
		block_dfs(arena, node->block, value, all);
}

void resolve_joint(struct arena *arena, struct joint *joint)
{
	struct block *block;

	while (joint->gen) {
		block = joint->gen;
		if (block->shape.type == SHAPE_BOX) {
			arena->move_orig_block = block;
			return;
		} else if (block->shape.type == SHAPE_WHEEL) {
			joint = block->shape.wheel.center;
		}
	}

	arena->move_orig_joint = joint;
}

void mouse_down_move(struct arena *arena, float x, float y)
{
	arena->move_orig_joint = NULL;
	arena->move_orig_block = NULL;

	if (arena->hover_joint) {
		resolve_joint(arena, arena->hover_joint);
		arena->state = STATE_MOVE;
		arena->move_orig_x = x;
		arena->move_orig_y = y;
		if (arena->move_orig_joint)
			joint_dfs(arena, arena->move_orig_joint, true, false);
		else
			block_dfs(arena, arena->move_orig_block, true, false);
	} else if (arena->hover_block) {
		arena->state = STATE_MOVE;
		arena->move_orig_x = x;
		arena->move_orig_y = y;
		arena->move_orig_block = arena->hover_block;
		block_dfs(arena, arena->hover_block, true, true);
	} else {
		arena->state = STATE_NORMAL_PAN;
	}
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
	block->visited = false;

	if (solid) {
		block->r = solid_rod_r;
		block->g = solid_rod_g;
		block->b = solid_rod_b;
	} else {
		block->r = water_rod_r;
		block->g = water_rod_g;
		block->b = water_rod_b;
	}

	arena->new_block = block;

	gen_block(arena->world, block);

	append_block(&design->player_blocks, block);

	arena->hover_joint = j0;

	arena->state = STATE_NEW_ROD;

	mark_overlaps(arena);
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

	switch (arena->tool) {
	case TOOL_WHEEL:
		block->shape.wheel.spin = 0;
		break;
	case TOOL_CW_WHEEL:
		block->shape.wheel.spin = 5;
		break;
	case TOOL_CCW_WHEEL:
		block->shape.wheel.spin = -5;
		break;
	}

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
	block->visited = false;

	switch (arena->tool) {
	case TOOL_WHEEL:
		block->r = wheel_r;
		block->g = wheel_g;
		block->b = wheel_b;
		break;
	case TOOL_CW_WHEEL:
		block->r = cw_wheel_r;
		block->g = cw_wheel_g;
		block->b = cw_wheel_b;
		break;
	case TOOL_CCW_WHEEL:
		block->r = ccw_wheel_r;
		block->g = ccw_wheel_g;
		block->b = ccw_wheel_b;
		break;
	}

	arena->new_block = block;

	gen_block(arena->world, block);

	append_block(&design->player_blocks, block);

	arena->hover_joint = j0;

	arena->state = STATE_NEW_WHEEL;

	mark_overlaps(arena);
}

bool inside_area(struct area *area, double x, double y)
{
	double w_half = area->w / 2;
	double h_half = area->h / 2;

	return x > area->x - w_half
	    && x < area->x + w_half
	    && y > area->y - h_half
	    && y < area->y + h_half;
}

void mouse_down_tool(struct arena *arena, float x, float y)
{
	switch (arena->tool) {
	case TOOL_MOVE:
		mouse_down_move(arena, x, y);
		break;
	case TOOL_ROD:
	case TOOL_SOLID_ROD:
		mouse_down_rod(arena, x, y);
		break;
	case TOOL_WHEEL:
	case TOOL_CW_WHEEL:
	case TOOL_CCW_WHEEL:
		mouse_down_wheel(arena, x, y);
		break;
	case TOOL_DELETE:
		mouse_down_delete(arena, x, y);
		break;
	}
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

	switch (arena->state) {
	case STATE_NORMAL:
		if (inside_area(&arena->design.build_area, x_world, y_world))
			mouse_down_tool(arena, x_world, y_world);
		else
			arena->state = STATE_NORMAL_PAN;
		break;
	case STATE_RUNNING:
		arena->state = STATE_RUNNING_PAN;
		break;
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
