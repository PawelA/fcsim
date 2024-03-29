#include <stdio.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <fcsim.h>
#include <fcsim_funcs.h>

#include "yxml/yxml.h"

struct str_slice {
	char *ptr;
	int len;
};

struct state {
	struct str_slice str;
	yxml_t yxml;
	yxml_ret_t cur;
	fcsim_arena *arena;
	fcsim_block_def *block;
};

static char yxml_buf[1024];

static void next(state *st)
{
again:
	if (st->str.len > 0) {
		st->cur = yxml_parse(&st->yxml, *st->str.ptr);
		st->str.ptr++;
		st->str.len--;
	} else {
		st->cur = yxml_eof(&st->yxml);
		return;
	}

	if (st->cur == YXML_OK)
		goto again;
	/* TODO: only ignore whitespace at the start of the content */
	if (st->cur == YXML_CONTENT && isspace(st->yxml.data[0]))
		goto again;
}

static bool ignore_field(state *st)
{
	/* TODO: keep a balance of ELEMSTART and ELEMEND instead of using recursion */
	next(st);
	while (true) {
		switch (st->cur) {
		case YXML_CONTENT:
			next(st);
			break;
		case YXML_ELEMSTART:
			ignore_field(st);
			break;
		case YXML_ELEMEND:
			next(st);
			return true;
		default:
			return false;
		}
	}
}

static bool read_string(state *st, struct str_slice *str)
{
	char *start = st->str.ptr;
	char *end = start;

	next(st);
	while (st->cur == YXML_CONTENT) {
		if (!isspace(st->yxml.data[0]))
			end = st->str.ptr;
		next(st);
	}

	str->ptr = start;
	str->len = end - start;

	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	return true;
}

static bool read_int(state *st, int *res)
{
	struct str_slice str;

	if (!read_string(st, &str))
		return false;

	fcsim_strtoi(str.ptr, str.len, res);

	return true;
}

static bool read_number(state *st, double *res)
{
	struct str_slice str;

	if (!read_string(st, &str))
		return false;

	fcsim_strtod(str.ptr, str.len, res);

	return true;
}

static int str_equal_to(struct str_slice *str, const char *ns)
{
	int nlen = strlen(ns);

	return str->len == nlen && memcmp(str->ptr, ns, nlen) == 0;
}

static bool read_bool(state *st, bool *res)
{
	struct str_slice str;

	if (!read_string(st, &str))
		return false;

	if (str_equal_to(&str, "true"))
		*res = true;
	else if (str_equal_to(&str, "false"))
		*res = false;
	else
		return false;

	return true;
}

static bool read_position(state *st, double *x, double *y)
{
	next(st);
	while (st->cur == YXML_ELEMSTART) {
		if (!strcmp(st->yxml.elem, "x")) {
			if (!read_number(st, x))
				return false;
		} else if (!strcmp(st->yxml.elem, "y")) {
			if (!read_number(st, y))
				return false;
		} else {
			if (!ignore_field(st))
				return false;
		}
	}
	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	return true;
}

enum xml_block_type {
	STATIC_RECTANGLE,
	STATIC_CIRCLE,
	DYNAMIC_RECTANGLE,
	DYNAMIC_CIRCLE,
	JOINTED_DYNAMIC_RECTANGLE,
	NO_SPIN_WHEEL,
	CLOCKWISE_WHEEL,
	COUNTER_CLOCKWISE_WHEEL,
	SOLID_ROD,
	HOLLOW_ROD,
};

static bool read_joints(state *st, fcsim_block_def *block)
{
	next(st);
	while (st->cur == YXML_ELEMSTART) {
		if (!strcmp(st->yxml.elem, "jointedTo")) {
			if (!read_int(st, &block->joints[block->joint_cnt++]))
				return false;
		} else {
			if (!ignore_field(st))
				return false;
		}
	}
	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	return true;
}

static bool ignore_attr(state *st)
{
	next(st);
	while (st->cur == YXML_ATTRVAL)
		next(st);
	if (st->cur != YXML_ATTREND)
		return false;
	next(st);

	return true;
}

static bool read_id(state *st, int *id)
{
	int res = 0;
	
	next(st);
	while (st->cur == YXML_ATTRVAL) {
		/* TODO: check the digits */
		res = res * 10 + (st->yxml.data[0] - '0');
		next(st);
	}
	if (st->cur != YXML_ATTREND)
		return false;
	next(st);

	*id = res;
	return true;
}

static bool read_block_attrs(state *st, fcsim_block_def *block)
{
	while (st->cur == YXML_ATTRSTART) {
		bool res;
		if (!strcmp(st->yxml.attr, "id"))
			res = read_id(st, &block->id);
		else
			res = ignore_attr(st);
		if (!res)
			return false;
	}

	return true;
}

static bool read_block(state *st, xml_block_type type)
{
	fcsim_block_def *block = st->block++;
	bool goal_block = false;

	next(st);

	memset(block, 0, sizeof(*block));
	block->id = -1;
	if (!read_block_attrs(st, block))
		return false;

	while (st->cur == YXML_ELEMSTART) {
		if (!strcmp(st->yxml.elem, "rotation")) {
			if (!read_number(st, &block->angle))
				return false;
		} else if (!strcmp(st->yxml.elem, "position")) {
			if (!read_position(st, &block->x, &block->y))
				return false;
		} else if (!strcmp(st->yxml.elem, "width")) {
			if (!read_number(st, &block->w))
				return false;
		} else if (!strcmp(st->yxml.elem, "height")) {
			if (!read_number(st, &block->h))
				return false;
		} else if (!strcmp(st->yxml.elem, "goalBlock")) {
			if (!read_bool(st, &goal_block))
				return false;
		} else if (!strcmp(st->yxml.elem, "joints")) {
			if (!read_joints(st, block))
				return false;
		} else {
			if (!ignore_field(st))
				return false;
		}
	}
	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	switch (type) {
	case STATIC_RECTANGLE:
		block->type = FCSIM_STAT_RECT;
		break;
	case STATIC_CIRCLE:
		block->type = FCSIM_STAT_CIRCLE;
		break;
	case DYNAMIC_RECTANGLE:
		block->type = FCSIM_DYN_RECT;
		break;
	case DYNAMIC_CIRCLE:
		block->type = FCSIM_DYN_CIRCLE;
		break;
	case JOINTED_DYNAMIC_RECTANGLE:
		block->type = FCSIM_GOAL_RECT;
		break;
	case NO_SPIN_WHEEL:
		if (goal_block)
			block->type = FCSIM_GOAL_CIRCLE;
		else
			block->type = FCSIM_WHEEL;
		break;
	case CLOCKWISE_WHEEL:
		block->type = FCSIM_CW_WHEEL;
		break;
	case COUNTER_CLOCKWISE_WHEEL:
		block->type = FCSIM_CCW_WHEEL;
		break;
	case SOLID_ROD:
		block->type = FCSIM_SOLID_ROD;
		break;
	case HOLLOW_ROD:
		block->type = FCSIM_ROD;
		break;
	}

	return true;
}

static bool read_blocks(state *st)
{
	next(st);
	while (st->cur == YXML_ELEMSTART) {
		if (!strcmp(st->yxml.elem, "StaticRectangle")) {
			if (!read_block(st, STATIC_RECTANGLE))
				return false;
		} else if (!strcmp(st->yxml.elem, "StaticCircle")) {
			if (!read_block(st, STATIC_CIRCLE))
				return false;
		} else if (!strcmp(st->yxml.elem, "DynamicRectangle")) {
			if (!read_block(st, DYNAMIC_RECTANGLE))
				return false;
		} else if (!strcmp(st->yxml.elem, "DynamicCircle")) {
			if (!read_block(st, DYNAMIC_CIRCLE))
				return false;
		} else if (!strcmp(st->yxml.elem, "JointedDynamicRectangle")) {
			if (!read_block(st, JOINTED_DYNAMIC_RECTANGLE))
				return false;
		} else if (!strcmp(st->yxml.elem, "NoSpinWheel")) {
			if (!read_block(st, NO_SPIN_WHEEL))
				return false;
		} else if (!strcmp(st->yxml.elem, "ClockwiseWheel")) {
			if (!read_block(st, CLOCKWISE_WHEEL))
				return false;
		} else if (!strcmp(st->yxml.elem, "CounterClockwiseWheel")) {
			if (!read_block(st, COUNTER_CLOCKWISE_WHEEL))
				return false;
		} else if (!strcmp(st->yxml.elem, "SolidRod")) {
			if (!read_block(st, SOLID_ROD))
				return false;
		} else if (!strcmp(st->yxml.elem, "HollowRod")) {
			if (!read_block(st, HOLLOW_ROD))
				return false;
		} else {
			if (!ignore_field(st))
				return false;
		}
	}
	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	return true;
}

static bool read_area(state *st, fcsim_rect *area)
{
	next(st);
	while (st->cur == YXML_ELEMSTART) {
		if (!strcmp(st->yxml.elem, "position")) {
			if (!read_position(st, &area->x, &area->y))
				return false;
		} else if (!strcmp(st->yxml.elem, "width")) {
			if (!read_number(st, &area->w))
				return false;
		} else if (!strcmp(st->yxml.elem, "height")) {
			if (!read_number(st, &area->h))
				return false;
		} else {
			if (!ignore_field(st))
				return false;
		}
	}
	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	return true;
}

static bool read_level(state *st)
{
	next(st);
	while (st->cur == YXML_ELEMSTART) {
		if (!strcmp(st->yxml.elem, "levelBlocks")) {
			if (!read_blocks(st))
				return false;
		} else if (!strcmp(st->yxml.elem, "playerBlocks")) {
			if (!read_blocks(st))
				return false;
		} else if (!strcmp(st->yxml.elem, "start")) {
			if (!read_area(st, &st->arena->build))
				return false;
		} else if (!strcmp(st->yxml.elem, "end")) {
			if (!read_area(st, &st->arena->goal))
				return false;
		} else {
			if (!ignore_field(st))
				return false;
		}
	}
	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	return true;
}

static bool read_retrieve_level(state *st)
{
	if (st->cur != YXML_ELEMSTART)
		return false;
	if (strcmp(st->yxml.elem, "retrieveLevel"))
		return false;

	next(st);
	while (st->cur == YXML_ELEMSTART) {
		if (!strcmp(st->yxml.elem, "level")) {
			if (!read_level(st))
				return false;
		} else {
			if (!ignore_field(st))
				return false;
		}
	}
	if (st->cur != YXML_ELEMEND)
		return false;
	next(st);

	return true;
}

void sanitize(char *xml, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (xml[i] == '&')
			xml[i] = '_';
	}
}

int fcsim_read_xml(char *xml, int len, fcsim_arena *arena)
{
	state st;

	sanitize(xml, len);

	arena->block_cnt = 1000;
	arena->blocks = new fcsim_block_def[arena->block_cnt];

	yxml_init(&st.yxml, yxml_buf, sizeof(yxml_buf));
	st.str.ptr = xml;
	st.str.len = len;
	st.arena = arena;
	st.block = arena->blocks;

	next(&st);
	if (!read_retrieve_level(&st))
		return -1;

	arena->block_cnt = st.block - arena->blocks;

	return 0;
}
