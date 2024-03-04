#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GLFW/glfw3.h>
#include <fcsim.h>

#include "ui.h"
#include "text.h"
#include "globals.h"
#include "event.h"
#include "runner.h"
#include "load_layer.h"
#include "arena_layer.h"

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

	if (shape->type == FCSIMN_SHAPE_CIRC)
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

void arena_layer_init(struct arena_layer *arena_layer)
{
	arena_layer->loaded = 0;
	button_init(&arena_layer->load_button, "Load", 10, 10);
	load_layer_init(&arena_layer->load_layer);
	arena_layer->runner = runner_create();
	arena_layer->running = 0;
	arena_layer->fast = 0;
	arena_layer->view_scale = 1.0;
	set_view_wh_from_scale(&arena_layer->view, 1.0);
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

void arena_layer_draw(struct arena_layer *arena_layer)
{
	glClear(GL_COLOR_BUFFER_BIT);

	if (!arena_layer->load_layer.open) {
		if (arena_layer->load_layer.loaded) {
			struct fcsim_where dummy;
			int i;

			arena_layer->level = arena_layer->load_layer.level;
			arena_layer->player_shapes = malloc(arena_layer->level.player_block_cnt * sizeof(struct fcsim_shape));
			arena_layer->level_shapes = malloc(arena_layer->level.level_block_cnt * sizeof(struct fcsim_shape));
			arena_layer->player_wheres = malloc(arena_layer->level.player_block_cnt * sizeof(struct fcsim_where));
			arena_layer->level_wheres = malloc(arena_layer->level.level_block_cnt * sizeof(struct fcsim_where));
			arena_layer_update_descs(arena_layer);
			arena_layer->loaded = 1;
			arena_layer->load_layer.loaded = 0;
		}
	}

	if (arena_layer->loaded) {
		if (arena_layer->running) {
			struct runner_tick tick;
			uint64_t won_tick;

			runner_get_tick(arena_layer->runner, &tick);
			won_tick = runner_get_won_tick(arena_layer->runner);

			view = arena_layer->view;
			draw_level(arena_layer, &tick);
			text_draw_uint64(tick.index, 200, 10, the_ui_scale);
			if (won_tick)
				text_draw_uint64(won_tick, 200, 50, the_ui_scale);
		} else {
			struct runner_tick tick;

			view = arena_layer->view;
			tick.player_wheres = arena_layer->player_wheres;
			tick.level_wheres = arena_layer->level_wheres;
			draw_level(arena_layer, &tick);
		}
	}

	draw_button(&arena_layer->load_button);
	if (arena_layer->load_layer.open)
		load_layer_draw(&arena_layer->load_layer);
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

void arena_layer_key_event(struct arena_layer *arena_layer, struct key_event *event)
{
	if (!arena_layer->loaded)
		return;

	if (event->key == GLFW_KEY_SPACE && event->action == GLFW_PRESS) {
		if (arena_layer->running) {
			runner_stop(arena_layer->runner);
		} else {
			runner_load(arena_layer->runner, &arena_layer->level);
			runner_start(arena_layer->runner);
		}
		arena_layer->running = !arena_layer->running;
	}

	if (event->key == GLFW_KEY_S && event->action == GLFW_PRESS)
		arena_layer_toggle_fast(arena_layer);
}

void arena_layer_mouse_move_event(struct arena_layer *arena_layer)
{
	int x = the_cursor_x;
	int y = the_cursor_y;

	if (button_hit_test(&arena_layer->load_button, x, y)) {
		arena_layer->load_button.state = BUTTON_HOVERED;
	} else {
		arena_layer->load_button.state = BUTTON_NORMAL;
	}

	int dx_pixel = x - arena_layer->prev_x;
	int dy_pixel = y - arena_layer->prev_y;
	float dx_world = ((float)dx_pixel / the_width) * arena_layer->view.w_half * 2;
	float dy_world = ((float)dy_pixel / the_height) * arena_layer->view.h_half * 2;

	switch (arena_layer->drag.type) {
	case DRAG_PAN:
		arena_layer->view.x -= dx_world;
		arena_layer->view.y -= dy_world;
		break;
	case DRAG_MOVE_JOINT:
		arena_layer->level.vertices[arena_layer->drag.vertex_id].x += dx_world;
		arena_layer->level.vertices[arena_layer->drag.vertex_id].y += dy_world;
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

int vertex_hit_test(struct fcsim_level *level, float x, float y)
{
	struct fcsim_vertex *vert;
	int i;

	for (i = level->vertex_cnt - 1; i >= 0; i--) {
		vert = &level->vertices[i];
		if (distance(vert->x, vert->y, x, y) < 10.0f)
			return i;
	}

	return -1;
}

int rect_hit_test(struct fcsim_shape *shape, struct fcsim_where *where, float x, float y)
{
	return 0;
}

int circ_hit_test(struct fcsim_shape *shape, struct fcsim_where *where, float x, float y)
{
	return distance(where->x, where->y, x, y) < shape->circ.radius;
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
		if (shape->type == FCSIMN_SHAPE_CIRC)
			res = circ_hit_test(shape, where, x, y);
		else
			res = rect_hit_test(shape, where, x, y);
		if (res)
			return i;
	}

	return -1;
}

void arena_layer_mouse_button_event(struct arena_layer *arena_layer, struct mouse_button_event *event)
{
	int x = the_cursor_x;
	int y = the_cursor_y;
	float x_world;
	float y_world;
	int vert_id;

	if (event->button == GLFW_MOUSE_BUTTON_LEFT && event->action == GLFW_PRESS) {
		if (button_hit_test(&arena_layer->load_button, x, y)) {
			arena_layer->load_button.state = BUTTON_NORMAL;
			arena_layer->load_layer.open = 1;
			return;
		}
	}

	pixel_to_world(&arena_layer->view, x, y, &x_world, &y_world);

	if (arena_layer->running) {
		if (event->button == GLFW_MOUSE_BUTTON_LEFT) {
			if (event->action == GLFW_PRESS)
				arena_layer->drag.type = DRAG_PAN;
			else
				arena_layer->drag.type = DRAG_NONE;
		}
	} else {
		if (event->button == GLFW_MOUSE_BUTTON_LEFT) {
			if (event->action == GLFW_PRESS) {
				vert_id = vertex_hit_test(&arena_layer->level, x_world, y_world);
				if (vert_id == -1) {
					arena_layer->drag.type = DRAG_PAN;
				} else {
					arena_layer->drag.type = DRAG_MOVE_JOINT;
					arena_layer->drag.vertex_id = vert_id;
				}
			} else {
				arena_layer->drag.type = DRAG_NONE;
			}
		}
	}
}

void arena_layer_scroll_event(struct arena_layer *arena_layer, struct scroll_event *event)
{
	float scale = 1.0f - event->delta * 0.05f;

	arena_layer->view_scale *= scale;
	set_view_wh_from_scale(&arena_layer->view, arena_layer->view_scale);
}

void arena_layer_size_event(struct arena_layer *arena_layer)
{
	set_view_wh_from_scale(&arena_layer->view, arena_layer->view_scale);
}

void arena_layer_event(struct arena_layer *arena_layer, struct event *event)
{
	if (arena_layer->load_layer.open) {
		load_layer_event(&arena_layer->load_layer, event);
		return;
	}

	switch (event->type) {
	case EVENT_KEY:
		arena_layer_key_event(arena_layer, &event->key_event);
		break;
	case EVENT_MOUSE_MOVE:
		arena_layer_mouse_move_event(arena_layer);
		break;
	case EVENT_MOUSE_BUTTON:
		arena_layer_mouse_button_event(arena_layer, &event->mouse_button_event);
		break;
	case EVENT_SCROLL:
		arena_layer_scroll_event(arena_layer, &event->scroll_event);
		break;
	case EVENT_SIZE:
		arena_layer_size_event(arena_layer);
		break;
	}
}
