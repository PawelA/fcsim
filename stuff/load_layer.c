#include <string.h>
#include <GLFW/glfw3.h>

#include <fcsim.h>
#include "ui.h"
#include "event.h"
#include "text.h"
#include "globals.h"
#include "loader.h"
#include "load_layer.h"

void load_layer_init(struct load_layer *load_layer)
{
	load_layer->open = 0;
	load_layer->loading = 0;
	load_layer->loader = NULL;
	memset(load_layer->buf, 0, 16);
	input_box_init(&load_layer->input, load_layer->buf, 16);
}

void load_layer_draw(struct load_layer *load_layer)
{
	draw_input_box(&load_layer->input, 10, 70, the_ui_scale);
	if (load_layer->loading) {
		if (loader_is_done(load_layer->loader)) {
			loader_get(load_layer->loader, &load_layer->level);
			load_layer->loaded = 1;
			load_layer->loader = NULL;
			load_layer->loading = 0;
			load_layer->open = 0;
			memset(load_layer->buf, 0, 16);
			input_box_init(&load_layer->input, load_layer->buf, 16);
		} else {
			text_draw_str("Loading...", 20, 125, the_ui_scale);
		}
	}
}

void load_layer_key_event(struct load_layer *load_layer, struct key_event *event)
{
	if (load_layer->loading) {
		if (event->key == GLFW_KEY_ESCAPE && event->action == GLFW_PRESS) {
			load_layer->loading = 0;
			load_layer->loader = NULL;
		}
		return;
	}

	if (event->key == GLFW_KEY_ESCAPE && event->action == GLFW_PRESS)
		load_layer->open = 0;
	
	if (event->key == GLFW_KEY_ENTER && event->action == GLFW_PRESS) {
		load_layer->loading = 1;
		load_layer->loader = loader_load(load_layer->input.buf);
	}

	if (event->action == GLFW_PRESS || event->action == GLFW_REPEAT) {
		if (event->key == GLFW_KEY_BACKSPACE)
			input_box_backspace(&load_layer->input);
		if (event->key == GLFW_KEY_LEFT)
			input_box_move_cursor(&load_layer->input, -1);
		if (event->key == GLFW_KEY_RIGHT)
			input_box_move_cursor(&load_layer->input, +1);
	}

	if (event->key == GLFW_KEY_V && event->action == GLFW_PRESS) {
		const char *str;

		str = glfwGetClipboardString(the_window);
		for (; *str; str++)
			input_box_add_char(&load_layer->input, *str);
	}
}

void load_layer_char_event(struct load_layer *load_layer, struct char_event *event)
{
	if (load_layer->loading)
		return;

	if (event->codepoint >= 32 && event->codepoint <= 127)
		input_box_add_char(&load_layer->input, event->codepoint);
}

void load_layer_event(struct load_layer *load_layer, struct event *event)
{
	switch (event->type) {
	case EVENT_KEY:
		load_layer_key_event(load_layer, &event->key_event);
		break;
	case EVENT_MOUSE_MOVE:
		break;
	case EVENT_MOUSE_BUTTON:
		break;
	case EVENT_CHAR:
		load_layer_char_event(load_layer, &event->char_event);
	}
}
