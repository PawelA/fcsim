#include <stdint.h>

#include "fcsim.h"
#include "text.h"
#include "arena_layer.h"

int the_width = 800;
int the_height = 800;
int the_cursor_x = 0;
int the_cursor_y = 0;

struct arena_layer the_arena_layer;

void key_down(int key)
{
	arena_layer_key_down_event(&the_arena_layer, key);
}

void key_up(int key)
{
	arena_layer_key_up_event(&the_arena_layer, key);
}

void move(int x, int y)
{
	the_cursor_x = x;
	the_cursor_y = y;

	arena_layer_mouse_move_event(&the_arena_layer);
}

void button_down(int button)
{
	arena_layer_mouse_button_down_event(&the_arena_layer, button);
}

void button_up(int button)
{
	arena_layer_mouse_button_up_event(&the_arena_layer, button);
}

void scroll(int delta)
{
	arena_layer_scroll_event(&the_arena_layer, delta);
}

void resize(int w, int h)
{
	the_width = w;
	the_height = h;
	glViewport(0, 0, w, h);

	arena_layer_size_event(&the_arena_layer);
}

void init(void)
{
	arena_layer_init(&the_arena_layer);
	text_setup_draw();
	arena_layer_show(&the_arena_layer);
}

void draw(void)
{
	arena_layer_draw(&the_arena_layer);
}
