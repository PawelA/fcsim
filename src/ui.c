#include <stdio.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include "ui.h"
#include "text.h"
#include "globals.h"

static void vertex2f_pixel(int x, int y)
{
	glVertex2f((float)(2*x - the_width) / the_width,
		   (float)(the_height - 2*y) / the_height);
}

void input_box_init(struct input_box *input, char *buf, int len)
{
	input->buf = buf;
	input->len = len;
	input->pos = 0;
}

void input_box_add_char(struct input_box *input, char c)
{
	int text_len;
	int i;

	text_len = strlen(input->buf);
	if (text_len == input->len)
		return;

	for (i = text_len; i > input->pos; i--)
		input->buf[i] = input->buf[i-1];
	input->buf[input->pos] = c;
	input->pos++;
}

void input_box_backspace(struct input_box *input)
{
	int text_len;
	int i;

	if (input->pos == 0)
		return;
	text_len = strlen(input->buf);

	for (i = input->pos; i <= text_len; i++)
		input->buf[i-1] = input->buf[i];
	input->pos--;
}

void input_box_move_cursor(struct input_box *input, int delta)
{
	int text_len;
	int pos;

	pos = input->pos;
	text_len = strlen(input->buf);
	pos += delta;
	if (pos < 0)
		pos = 0;
	if (pos > text_len)
		pos = text_len;
	input->pos = pos;
}

void draw_box(int x, int y, int w, int h, float c)
{
	glEnable(GL_BLEND);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(c, c, c, 0.5f);
	vertex2f_pixel(x, y + h);
	vertex2f_pixel(x, y);
	vertex2f_pixel(x + w, y);
	vertex2f_pixel(x + w, y + h);
	glEnd();
	glDisable(GL_BLEND);
}

static void draw_cursor(int x, int y, int scale)
{
	int w = scale;
	int h = scale * 8;

	glEnable(GL_BLEND);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	vertex2f_pixel(x, y + h);
	vertex2f_pixel(x, y);
	vertex2f_pixel(x + w, y);
	vertex2f_pixel(x + w, y + h);
	glEnd();
	glDisable(GL_BLEND);
}

void draw_input_box(struct input_box *input, int x, int y, int scale)
{
	int cx = x;
	int i;

	int tw = scale * (input->len * 6 + 1);
	int th = scale * 10;
	draw_box(x - scale, y - scale, tw, th, 0.5f);

	text_draw_str(input->buf, x, y, scale);
	/*
	for (i = 0; i < input->len; i++) {
		draw_char(input->buf[i], cx, y, scale);
		cx += 6 * scale;
	}
	*/
	draw_cursor(x + scale * (input->pos * 6 - 1), y, scale);
}

void button_init(struct button *button, char *text, int x, int y)
{
	button->text = text;
	button->len = strlen(text);
	button->x = x;
	button->y = y;
	button->state = BUTTON_NORMAL;
}

int button_hit_test(struct button *button, int x, int y)
{
	int xl = button->x - the_ui_scale;
	int yl = button->y - the_ui_scale;
	int w = the_ui_scale * (button->len * 6 + 1);
	int h = the_ui_scale * 10;

	if (x < xl)
		return 0;
	if (y < yl)
		return 0;
	if (x > xl + w)
		return 0;
	if (y > yl + h)
		return 0;

	return 1;
}

void draw_button(struct button *button)
{
	int tw = the_ui_scale * (button->len * 6 + 1);
	int th = the_ui_scale * 10;
	float c;

	if (button->state == BUTTON_HOVERED)
		c = 0.8f;
	else
		c = 0.5f;

	draw_box(button->x - the_ui_scale, button->y - the_ui_scale, tw, th, c);
	text_draw_str(button->text, button->x, button->y, the_ui_scale);
}
