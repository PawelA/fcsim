#include <stdio.h>
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
	{ 0.000f, 0.745f, 0.004f }, // FCSIM_STAT_CIRCLE
	{ 0.976f, 0.855f, 0.184f }, // FCSIM_DYN_RECT
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
	if (block->type == FCSIM_STAT_CIRCLE) {
		glColor3f(color->r, color->g, color->b);
	} else {
		glColor3f(color->r * COLOR_SCALE,
			  color->g * COLOR_SCALE,
			  color->b * COLOR_SCALE);
	}
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

void draw_arena(struct fcsim_arena *arena, struct fcsim_block_stat *stats)
{
	int i;

	draw_area(&arena->build, &build_color);
	draw_area(&arena->goal,  &goal_color);
	for (i = 0; i < arena->block_cnt; i++)
		draw_block(&arena->blocks[i], &stats[i]);
}

void arena_layer_init(struct arena_layer *arena_layer)
{
	arena_layer->loaded = 0;
	button_init(&arena_layer->load_button, "Load", 10, 10);
	load_layer_init(&arena_layer->load_layer);
	arena_layer->runner = runner_create();
	arena_layer->running = 0;
	arena_layer->min_frame_time = 16667;
    runner_set_frame_limit(arena_layer->runner, arena_layer->min_frame_time);
	arena_layer->view_scale = 1.0;
	set_view_wh_from_scale(&arena_layer->view, 1.0);
}

void arena_layer_draw(struct arena_layer *arena_layer)
{
	glClear(GL_COLOR_BUFFER_BIT);

	if (!arena_layer->load_layer.open) {
		if (arena_layer->load_layer.loaded) {
			arena_layer->arena = arena_layer->load_layer.arena;
			arena_layer->loaded = 1;
			arena_layer->pressed = 0;
			arena_layer->load_layer.loaded = 0;
			runner_load(arena_layer->runner, &arena_layer->arena);
		}
	}

	if (arena_layer->loaded) {
		struct runner_tick tick;
		uint64_t won_tick;

		runner_get_tick(arena_layer->runner, &tick);
		won_tick = runner_get_won_tick(arena_layer->runner);

		view = arena_layer->view;
		draw_arena(&arena_layer->arena, tick.stats);
		text_draw_uint64(tick.index, 200, 10, the_ui_scale);
		if (won_tick)
			text_draw_uint64(won_tick, 200, 50, the_ui_scale);
	}
	
	draw_button(&arena_layer->load_button);
	if (arena_layer->load_layer.open)
		load_layer_draw(&arena_layer->load_layer);
}

void arena_layer_show(struct arena_layer *arena_layer)
{
	glClearColor(0.529f, 0.741f, 0.945f, 0);
}

void arena_layer_toggle_running(struct arena_layer *arena_layer)
{
	if (arena_layer->running) {
		runner_stop(arena_layer->runner);
		runner_load(arena_layer->runner, &arena_layer->arena);
	} else {
		runner_start(arena_layer->runner);
	}
	arena_layer->running = !arena_layer->running;
}

void arena_layer_set_min_frame_time(struct arena_layer *arena_layer, uint64_t min_frame_time)
{
    arena_layer->min_frame_time = min_frame_time;
    runner_set_frame_limit(arena_layer->runner, arena_layer->min_frame_time);
}

void arena_layer_key_event(struct arena_layer *arena_layer, struct key_event *event)
{
	if (event->key == GLFW_KEY_SPACE && event->action == GLFW_PRESS)
		arena_layer_toggle_running(arena_layer);
	if (event->key == GLFW_KEY_1 && event->action == GLFW_PRESS)
		arena_layer_set_min_frame_time(arena_layer, 200000); //5fps
    if (event->key == GLFW_KEY_2 && event->action == GLFW_PRESS)
		arena_layer_set_min_frame_time(arena_layer, 66667); //15fps
    if (event->key == GLFW_KEY_3 && event->action == GLFW_PRESS)
		arena_layer_set_min_frame_time(arena_layer, 33333); //30fps 
    if (event->key == GLFW_KEY_4 && event->action == GLFW_PRESS)
		arena_layer_set_min_frame_time(arena_layer, 16667); //60fps
    if (event->key == GLFW_KEY_5 && event->action == GLFW_PRESS)
		arena_layer_set_min_frame_time(arena_layer, 8333); //60fps
    if (event->key == GLFW_KEY_6 && event->action == GLFW_PRESS)
		arena_layer_set_min_frame_time(arena_layer, 0); //unlimited fps
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
	float dx_world;
	float dy_world;

	if (arena_layer->pressed) {
		dx_world = ((float)dx_pixel / the_width) * arena_layer->view.w_half * 2;
		dy_world = ((float)dy_pixel / the_height) * arena_layer->view.h_half * 2;
		arena_layer->view.x -= dx_world;
		arena_layer->view.y -= dy_world;
	}

	arena_layer->prev_x = x;
	arena_layer->prev_y = y;
}

void arena_layer_mouse_button_event(struct arena_layer *arena_layer, struct mouse_button_event *event)
{
	int x = the_cursor_x;
	int y = the_cursor_y;

	if (event->button == GLFW_MOUSE_BUTTON_LEFT && event->action == GLFW_PRESS) {
		if (button_hit_test(&arena_layer->load_button, x, y)) {
			arena_layer->load_button.state = BUTTON_NORMAL;
			arena_layer->load_layer.open = 1;
		}
	}
	
	if (event->button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	if (event->action == GLFW_PRESS) {
		arena_layer->pressed = 1;
	} else {
		arena_layer->pressed = 0;
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
