#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GLFW/glfw3.h>

#include "fcsim.h"
#include "text.h"
#include "globals.h"
#include "runner.h"
#include "arena_layer.h"
#include "galois.h"

#define TAU 6.28318530718

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

static void set_view_wh_from_scale(struct arena_view *view, float scale)
{
	view->w_half = scale * the_width;
	view->h_half = scale * the_height;
}

struct arena_view view;

void vertex2f_world(double x, double y)
{
	glVertex2f((x - view.x) / view.w_half, (view.y - y) / view.h_half);
}

static void draw_rect(struct fcsim_shape_rect *rect,
		      struct fcsim_where *where,
		      struct color *color)
{
	double sina_half = sin(where->angle) / 2;
	double cosa_half = cos(where->angle) / 2;
	double w = fmax(fabs(rect->w), 4.0);
	double h = fmax(fabs(rect->h), 4.0);
	double wc = w * cosa_half;
	double ws = w * sina_half;
	double hc = h * cosa_half;
	double hs = h * sina_half;
	double x = where->x;
	double y = where->y;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(color->r, color->g, color->b);
	vertex2f_world( wc - hs + x,  ws + hc + y);
	vertex2f_world(-wc - hs + x, -ws + hc + y);
	vertex2f_world(-wc + hs + x, -ws - hc + y);
	vertex2f_world( wc + hs + x,  ws - hc + y);
	glEnd();
}

static void draw_area(struct fcsim_area *area, struct color *color)
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

static void draw_circ(struct fcsim_shape_circ *circ,
		      struct fcsim_where *where,
		      struct color *color)
{
	double x = where->x;
	double y = where->y;
	float r = circ->radius;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(color->r, color->g, color->b);
	vertex2f_world(x, y);
	for (int i = 0; i <= CIRCLE_SEGMENTS; i++) {
		double a = where->angle + TAU * i / CIRCLE_SEGMENTS;
		vertex2f_world(cos(a) * r + x, sin(a) * r + y);
	}
	glEnd();
}

static void draw_block(struct fcsim_shape *shape, struct fcsim_where *where, int type)
{
	struct color *color = &block_colors[type];

	if (shape->type == FCSIM_SHAPE_CIRC)
		draw_circ(&shape->circ, where, color);
	else
		draw_rect(&shape->rect, where, color);
}

void draw_level(struct arena_layer *arena_layer, struct runner_tick *tick)
{
	struct fcsim_level *level = &arena_layer->level;
	int i;

	draw_area(&level->build_area, &build_color);
	draw_area(&level->goal_area, &goal_color);

	for (i = 0; i < level->level_block_cnt; i++) {
		draw_block(&arena_layer->level_shapes[i],
			   &tick->level_wheres[i],
			   level->level_blocks[i].type);
	}

	for (i = 0; i < level->player_block_cnt; i++) {
		draw_block(&arena_layer->player_shapes[i],
			   &tick->player_wheres[i],
			   level->player_blocks[i].type);
	}
}

void arena_layer_update_descs(struct arena_layer *arena_layer)
{
	int i;

	for (i = 0; i < arena_layer->level.player_block_cnt; i++) {
		fcsim_get_player_block_desc(&arena_layer->level, i,
					     &arena_layer->player_shapes[i],
					     &arena_layer->player_wheres[i]);
	}

	for (i = 0; i < arena_layer->level.level_block_cnt; i++) {
		fcsim_get_level_block_desc(&arena_layer->level, i,
					    &arena_layer->level_shapes[i],
					    &arena_layer->level_wheres[i]);
	}
}

void arena_layer_init(struct arena_layer *arena_layer)
{
	arena_layer->runner = runner_create();
	arena_layer->running = 0;
	arena_layer->fast = 0;
	arena_layer->view_scale = 1.0;
	set_view_wh_from_scale(&arena_layer->view, 1.0);

	fcsim_parse_xml(galois_xml, sizeof(galois_xml), &arena_layer->level);

	arena_layer->player_shapes = malloc(arena_layer->level.player_block_cnt * sizeof(struct fcsim_shape));
	arena_layer->level_shapes = malloc(arena_layer->level.level_block_cnt * sizeof(struct fcsim_shape));
	arena_layer->player_wheres = malloc(arena_layer->level.player_block_cnt * sizeof(struct fcsim_where));
	arena_layer->level_wheres = malloc(arena_layer->level.level_block_cnt * sizeof(struct fcsim_where));
	arena_layer_update_descs(arena_layer);
}

void arena_layer_draw(struct arena_layer *arena_layer)
{
	glClear(GL_COLOR_BUFFER_BIT);

	if (arena_layer->running) {
		struct runner_tick tick;
		uint64_t won_tick;

		runner_get_tick(arena_layer->runner, &tick);
		won_tick = runner_get_won_tick(arena_layer->runner);

		view = arena_layer->view;
		draw_level(arena_layer, &tick);
		text_draw_uint64(tick.index, 500, 10, 4);
		if (won_tick)
			text_draw_uint64(won_tick, 500, 50, 4);
	} else {
		struct runner_tick tick;

		view = arena_layer->view;
		tick.player_wheres = arena_layer->player_wheres;
		tick.level_wheres = arena_layer->level_wheres;
		draw_level(arena_layer, &tick);
	}
}

void arena_layer_show(struct arena_layer *arena_layer)
{
	glClearColor(0.529f, 0.741f, 0.945f, 0);
}

void arena_layer_toggle_fast(struct arena_layer *arena_layer)
{
	arena_layer->fast = !arena_layer->fast;
	runner_set_frame_limit(arena_layer->runner, arena_layer->fast ? 0 : 16666);
}

void arena_layer_key_event(struct arena_layer *arena_layer, int key, int action)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		if (arena_layer->running) {
			runner_stop(arena_layer->runner);
		} else {
			runner_load(arena_layer->runner, &arena_layer->level);
			runner_start(arena_layer->runner);
		}
		arena_layer->running = !arena_layer->running;
	}

	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		arena_layer_toggle_fast(arena_layer);
}

void move_block(struct fcsim_level *level, int id, float dx, float dy)
{
	struct fcsim_block *block = &level->player_blocks[id];

	if (block->type == FCSIM_BLOCK_GOAL_RECT) {
		block->jrect.x += dx;
		block->jrect.y += dy;
	}
}

void arena_layer_mouse_move_event(struct arena_layer *arena_layer)
{
	int x = the_cursor_x;
	int y = the_cursor_y;

	int dx_pixel = x - arena_layer->prev_x;
	int dy_pixel = y - arena_layer->prev_y;
	float dx_world = ((float)dx_pixel / the_width) * arena_layer->view.w_half * 2;
	float dy_world = ((float)dy_pixel / the_height) * arena_layer->view.h_half * 2;

	switch (arena_layer->drag.type) {
	case DRAG_PAN:
		arena_layer->view.x -= dx_world;
		arena_layer->view.y -= dy_world;
		break;
	case DRAG_MOVE_VERTEX:
		arena_layer->level.vertices[arena_layer->drag.vertex_id].x += dx_world;
		arena_layer->level.vertices[arena_layer->drag.vertex_id].y += dy_world;
		arena_layer_update_descs(arena_layer);
		break;
	case DRAG_MOVE_BLOCK:
		move_block(&arena_layer->level, arena_layer->drag.block_id,
			   dx_world, dy_world);
		arena_layer_update_descs(arena_layer);
		break;
	}

	arena_layer->prev_x = x;
	arena_layer->prev_y = y;
}

void pixel_to_world(struct arena_view *view, int x, int y, float *x_world, float *y_world)
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

int joint_hit_test(struct fcsim_level *level, float x, float y, struct fcsim_joint *joint)
{
	struct fcsim_joint joints[5];
	int joint_cnt;
	double jx, jy;
	int i, j;

	for (i = level->player_block_cnt - 1; i >= 0; i--) {
		joint_cnt = fcsim_get_player_block_joints(level, i, joints);
		for (j = 0; j < joint_cnt; j++) {
			fcsim_get_joint_pos(level, &joints[j], &jx, &jy);
			if (distance(jx, jy, x, y) < 10.0f) {
				*joint = joints[j];
				return 1;
			}
		}
	}

	return 0;
}

int rect_hit_test(struct fcsim_shape_rect *rect,
		  struct fcsim_where *where, float x, float y)
{
	float dx = where->x - x;
	float dy = where->y - y;
	float s = sinf(where->angle);
	float c = cosf(where->angle);
	float dx_t =  dx * c + dy * s;
	float dy_t = -dx * s + dy * c;

	return fabsf(dx_t) < rect->w && fabsf(dy_t) < rect->h;
}

int circ_hit_test(struct fcsim_shape_circ *circ,
		  struct fcsim_where *where, float x, float y)
{
	float dx = where->x - x;
	float dy = where->y - y;
	float r = circ->radius;

	return dx * dx + dy * dy < r * r;
}

int player_block_hit_test(struct arena_layer *arena_layer, float x, float y)
{
	struct fcsim_level *level = &arena_layer->level;
	struct fcsim_shape *shape;
	struct fcsim_where *where;
	int res;
	int i;

	for (i = level->player_block_cnt - 1; i >= 0; i--) {
		shape = &arena_layer->player_shapes[i];
		where = &arena_layer->player_wheres[i];
		if (shape->type == FCSIM_SHAPE_CIRC)
			res = circ_hit_test(&shape->circ, where, x, y);
		else
			res = rect_hit_test(&shape->rect, where, x, y);
		if (res)
			return i;
	}

	return -1;
}

enum draggable_type {
	DRAGGABLE_VERTEX,
	DRAGGABLE_BLOCK,
};

struct draggable {
	enum draggable_type type;
	int id;
};

void resolve_draggable(struct fcsim_level *level,
		       struct fcsim_joint *joint, struct draggable *draggable)
{
	struct fcsim_block *block;
	
	if (joint->type == FCSIM_JOINT_FREE) {
		draggable->type = DRAGGABLE_VERTEX;
		draggable->id = joint->free.vertex_id;
		return;
	}

	block = &level->player_blocks[joint->derived.block_id];
	switch (block->type) {
	case FCSIM_BLOCK_GOAL_RECT:
		draggable->type = DRAGGABLE_BLOCK;
		draggable->id = joint->derived.block_id;
		break;
	case FCSIM_BLOCK_GOAL_CIRC:
	case FCSIM_BLOCK_WHEEL:
	case FCSIM_BLOCK_CW_WHEEL:
	case FCSIM_BLOCK_CCW_WHEEL:
		resolve_draggable(level, &block->wheel.center, draggable);
		break;
	}
}

void arena_layer_mouse_button_event(struct arena_layer *arena_layer, int button, int action)
{
	int x = the_cursor_x;
	int y = the_cursor_y;
	float x_world;
	float y_world;
	int vert_id;

	pixel_to_world(&arena_layer->view, x, y, &x_world, &y_world);

	if (arena_layer->running) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS)
				arena_layer->drag.type = DRAG_PAN;
			else
				arena_layer->drag.type = DRAG_NONE;
		}
	} else {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
				struct fcsim_joint joint;
				int res;

				res = joint_hit_test(&arena_layer->level, x_world, y_world, &joint);
				if (res) {
					struct draggable draggable;
					resolve_draggable(&arena_layer->level, &joint, &draggable);
					if (draggable.type == DRAGGABLE_VERTEX) {
						arena_layer->drag.type = DRAG_MOVE_VERTEX;
						arena_layer->drag.vertex_id = draggable.id;
					} else {
						arena_layer->drag.type = DRAG_MOVE_BLOCK;
						arena_layer->drag.block_id = draggable.id;
					}
				} else {
					arena_layer->drag.type = DRAG_PAN;
				}
			} else {
				arena_layer->drag.type = DRAG_NONE;
			}
		} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS) {
				//...
			}
		}
	}
}

void arena_layer_scroll_event(struct arena_layer *arena_layer, int delta)
{
	float scale = 1.0f - delta * 0.05f;

	arena_layer->view_scale *= scale;
	set_view_wh_from_scale(&arena_layer->view, arena_layer->view_scale);
}

void arena_layer_size_event(struct arena_layer *arena_layer)
{
	set_view_wh_from_scale(&arena_layer->view, arena_layer->view_scale);
}
