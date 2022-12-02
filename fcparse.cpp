#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "yxml/yxml.h"
#include "fcsim.h"

static char buf[4096];

yxml_t yxml;
const char *cur_str;
yxml_ret_t cur_ret;
struct fcsim_block_def *cur_block;
int cur_id;
int trash_int;
double trash_double;

static void next(void)
{
	do {
		if (!*cur_str) {
			cur_ret = yxml_eof(&yxml);
			if (cur_ret == YXML_OK)
				return;
		} else {
			cur_ret = yxml_parse(&yxml, *cur_str++);
		}
	} while (cur_ret == YXML_OK || (cur_ret == YXML_CONTENT && isspace(yxml.data[0])));
}

static void expect(yxml_ret_t ret)
{
	if (cur_ret != ret) {
		fprintf(stdout, "expected %d, got %d\n", ret, cur_ret);
		exit(1);
	}
}

static void expect_elem_start_exact(const char *name)
{
	expect(YXML_ELEMSTART);
	if (strcmp(yxml.elem, name)) {
		fprintf(stdout, "expected elem %s, got %s\n", name, yxml.elem);
		exit(1);
	}
}

static void expect_elem_end(void)
{
	expect(YXML_ELEMEND);
}

int read_string(void)
{
	int res = 0;

	while (cur_ret == YXML_CONTENT) {
		next();
		res++;
	}

	return res;
}

int read_int(void)
{
	const char *start = cur_str-1;
	char *end;
	int len;

	len = read_string();
	end = (char *)start + len;
	return strtol(start, &end, 10);
}

double read_double(void)
{
	const char *start = cur_str-1;
	char *end;
	int len;

	len = read_string();
	end = (char *)start + len;
	return strtod(start, &end);
}

void read_field(const char *name)
{
	expect_elem_start_exact(name);
	next();
	read_string();
	expect_elem_end();
	next();
}

double read_field_int(const char *name)
{
	int res;

	expect_elem_start_exact(name);
	next();
	res = read_int();
	expect_elem_end();
	next();

	return res;
}

double read_field_double(const char *name)
{
	double res;

	expect_elem_start_exact(name);
	next();
	res = read_double();
	expect_elem_end();
	next();

	return res;
}

void read_position(double *x, double *y)
{
	expect_elem_start_exact("position");
	next();
	*x = read_field_double("x");
	*y = read_field_double("y");
	expect_elem_end();
	next();
}

void read_joints(int *joints)
{
	expect_elem_start_exact("joints");
	next();
	while (cur_ret == YXML_ELEMSTART)
		*joints++ = read_field_int("jointedTo");
	expect_elem_end();
	next();
}

void toss_rotation_field(void)
{
	if (!strcmp(yxml.elem, "rotation"))
	{
		next();
		trash_double = read_double();
		expect_elem_end();
		next();
	}
}
void toss_tick_count(void)
{
	if (cur_ret == YXML_ELEMSTART) {
		//is this second check necessary?
		if (!strcmp(yxml.elem, "tickCount"))
		{
			next();
			trash_double = read_double();
			expect_elem_end();
			next();
		}
	}
}
void toss_piece_count(void)
{
	if (cur_ret == YXML_ELEMSTART) {
		//is this second check necessary?
		if (!strcmp(yxml.elem, "pieceCount"))
		{
			next();
			trash_double = read_double();
			expect_elem_end();
			next();
		}
	}
}

int block_type(const char *elem)
{
	if (!strcmp(elem, "StaticRectangle"))
		return FCSIM_STAT_RECT;
	if (!strcmp(elem, "StaticCircle"))
		return FCSIM_STAT_CIRCLE;
	if (!strcmp(elem, "DynamicRectangle"))
		return FCSIM_DYN_RECT;
	if (!strcmp(elem, "DynamicCircle"))
		return FCSIM_DYN_CIRCLE;
	if (!strcmp(elem, "NoSpinWheel"))
		return FCSIM_WHEEL;
	if (!strcmp(elem, "ClockwiseWheel"))
		return FCSIM_CW_WHEEL;
	if (!strcmp(elem, "CounterClockwiseWheel"))
		return FCSIM_CCW_WHEEL;
	if (!strcmp(elem, "SolidRod"))
		return FCSIM_SOLID_ROD;
	if (!strcmp(elem, "HollowRod"))
		return FCSIM_ROD;
	if (!strcmp(elem, "JointedDynamicRectangle"))
		return FCSIM_GOAL_RECT;
	return -1;
}

void read_block(void)
{
	expect(YXML_ELEMSTART);
	cur_block->type = block_type(yxml.elem);
	next();
	cur_block->id = -1;
	cur_block->joints[0] = -1;
	cur_block->joints[1] = -1;
	if (cur_ret == YXML_ATTRSTART) {
		cur_block->id = cur_id++;
		next();
		while (cur_ret == YXML_ATTRVAL)
			next();
		expect(YXML_ATTREND);
		next();
	}
	cur_block->angle = read_field_double("rotation");
	read_position(&cur_block->x, &cur_block->y);
	cur_block->w = read_field_double("width");
	cur_block->h = read_field_double("height");
	read_field("goalBlock");
	read_joints(cur_block->joints);
	expect(YXML_ELEMEND);
	next();
	cur_block++;
}

static void read_level_blocks(void)
{
	expect_elem_start_exact("levelBlocks");
	next();
	while (cur_ret == YXML_ELEMSTART) {
		read_block();
	}
	expect_elem_end();
	next();
}

static void read_player_blocks(void)
{
	expect_elem_start_exact("playerBlocks");
	next();
	while (cur_ret == YXML_ELEMSTART) {
		read_block();
	}
	expect_elem_end();
	next();
}

static void read_start(void)
{
	double x, y;
	double w, h;
	expect_elem_start_exact("start");
	next();
	toss_rotation_field();
	read_position(&x, &y);
	w = read_field_double("width");
	h = read_field_double("height");
	expect_elem_end();
	next();
	cur_block->type = FCSIM_START;
	cur_block->x = x;
	cur_block->y = y;
	cur_block->w = w;
	cur_block->h = h;
	cur_block->angle = 0;
	cur_block->id = -1;
	cur_block->joints[0] = -1;
	cur_block->joints[1] = -1;
	cur_block++;
}

static void read_end(void)
{
	double x, y;
	double w, h;
	expect_elem_start_exact("end");
	next();
	toss_rotation_field();
	read_position(&x, &y);
	w = read_field_double("width");
	h = read_field_double("height");
	expect_elem_end();
	next();
	cur_block->type = FCSIM_END;
	cur_block->x = x;
	cur_block->y = y;
	cur_block->w = w;
	cur_block->h = h;
	cur_block->angle = 0;
	cur_block->id = -1;
	cur_block->joints[0] = -1;
	cur_block->joints[1] = -1;
	cur_block++;
}

static void read_level(void)
{
	expect_elem_start_exact("level");
	next();
	read_level_blocks();
	read_player_blocks();
	read_start();
	read_end();
	toss_tick_count();
	toss_piece_count();
	expect_elem_end();
	next();
}

static void read_retrieve_level(void)
{
	expect_elem_start_exact("retrieveLevel");
	next();
	read_field("levelId");
	read_field("levelNumber");
	read_field("name");
	read_level();
	expect_elem_end();
	next();
}

int fcsim_parse_xml(const char *xml, struct fcsim_block_def *blocks)
{
	int res;

	yxml_init(&yxml, buf, sizeof(buf));

	cur_str = xml;
	cur_block = blocks;
	cur_id = 0;

	next();
	read_retrieve_level();
	expect(YXML_OK);

	return cur_block - blocks;
}
