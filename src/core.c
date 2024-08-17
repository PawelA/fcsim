#include <stdbool.h>
#include <stdint.h>
#include <box2d/b2Body.h>
#include "gl.h"
#include "graph.h"
#include "arena.h"

struct arena the_arena;

void key_down(int key)
{
	arena_key_down_event(&the_arena, key);
}

void key_up(int key)
{
	arena_key_up_event(&the_arena, key);
}

void move(int x, int y)
{
	arena_mouse_move_event(&the_arena, x, y);
}

void button_down(int button)
{
	arena_mouse_button_down_event(&the_arena, button);
}

void button_up(int button)
{
	arena_mouse_button_up_event(&the_arena, button);
}

void scroll(int delta)
{
	arena_scroll_event(&the_arena, delta);
}

void resize(int w, int h)
{
	glViewport(0, 0, w, h);

	arena_size_event(&the_arena, w, h);
}

void init(void)
{
	arena_compile_shaders();
	arena_init(&the_arena, 800, 800);
}

void draw(void)
{
	arena_draw(&the_arena);
}

void call(void (*func)(void *arg), void *arg)
{
	func(arg);
}
