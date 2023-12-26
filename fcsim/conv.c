#include <stdlib.h>
#include <math.h>

#include <xml.h>
#include <fcsimn.h>
#include <fcsim_funcs.h>

#include <stdio.h>
static void print_rect(struct fcsimn_block *block)
{
	switch (block->type) {
	case FCSIMN_BLOCK_STAT_RECT:
		printf("stat_rect");
		break;
	case FCSIMN_BLOCK_DYN_RECT:
		printf("dyn_rect");
		break;
	}
	printf("((%f %f), (%f %f), %f)\n",
		block->rect.x,
		block->rect.y,
		block->rect.w,
		block->rect.h,
		block->rect.angle);
}

static void print_circ(struct fcsimn_block *block)
{
	switch (block->type) {
	case FCSIMN_BLOCK_STAT_CIRC:
		printf("stat_circ");
		break;
	case FCSIMN_BLOCK_DYN_CIRC:
		printf("dyn_circ");
		break;
	}
	printf("((%f %f), %f)\n",
		block->circ.x,
		block->circ.y,
		block->circ.radius);
}

static void print_joint(struct fcsimn_joint *joint)
{
	switch (joint->type) {
	case FCSIMN_JOINT_FREE:
		printf("free(%d)", joint->free.vertex_id);
		break;
	case FCSIMN_JOINT_DERIVED:
		printf("derived(%d, %d)", joint->derived.block_id, joint->derived.index);
		break;
	}
}

static void print_jrect(struct fcsimn_block *block)
{
	printf("goal_rect((%f %f), (%f %f), %f)\n",
		block->jrect.x,
		block->jrect.y,
		block->jrect.w,
		block->jrect.h,
		block->jrect.angle);
}

static void print_wheel(struct fcsimn_block *block)
{
	switch (block->type) {
	case FCSIMN_BLOCK_GOAL_CIRC:
		printf("goal_circ");
		break;
	case FCSIMN_BLOCK_WHEEL:
		printf("wheel");
		break;
	case FCSIMN_BLOCK_CW_WHEEL:
		printf("cw_wheel");
		break;
	case FCSIMN_BLOCK_CCW_WHEEL:
		printf("ccw_wheel");
		break;
	}
	printf("(");
	print_joint(&block->wheel.center);
	printf(", %f, %f)\n", block->wheel.radius, block->wheel.angle);
}

static void print_rod(struct fcsimn_block *block)
{
	switch (block->type) {
	case FCSIMN_BLOCK_ROD:
		printf("rod");
		break;
	case FCSIMN_BLOCK_SOLID_ROD:
		printf("solid_rod");
		break;
	}
	printf("(");
	print_joint(&block->rod.from);
	printf(", ");
	print_joint(&block->rod.to);
	printf(")\n");
}

static void print_block(struct fcsimn_block *block)
{
	switch (block->type) {
	case FCSIMN_BLOCK_STAT_RECT:
	case FCSIMN_BLOCK_DYN_RECT:
		print_rect(block);
		break;
	case FCSIMN_BLOCK_STAT_CIRC:
	case FCSIMN_BLOCK_DYN_CIRC:
		print_circ(block);
		break;
	case FCSIMN_BLOCK_GOAL_RECT:
		print_jrect(block);
		break;
	case FCSIMN_BLOCK_GOAL_CIRC:
	case FCSIMN_BLOCK_WHEEL:
	case FCSIMN_BLOCK_CW_WHEEL:
	case FCSIMN_BLOCK_CCW_WHEEL:
		print_wheel(block);
		break;
	case FCSIMN_BLOCK_ROD:
	case FCSIMN_BLOCK_SOLID_ROD:
		print_rod(block);
		break;
	}
}

static void print_level(struct fcsimn_level *level)
{
	int i;

	printf("level blocks:\n");
	for (i = 0; i < level->level_block_cnt; i++)
		print_block(&level->level_blocks[i]);

	printf("player blocks:\n");
	for (i = 0; i < level->player_block_cnt; i++)
		print_block(&level->player_blocks[i]);
}

struct block_map_node {
	int id;
	int index;
	struct block_map_node *next;
};

struct block_map {
	struct block_map_node *head;
};

static void block_map_add(struct block_map *map, int id, int index)
{

}

static int block_map_find(struct block_map *map, int id)
{
	return id;
}

static void get_free_joint_pos(struct fcsimn_level *level,
			       struct fcsimn_free_joint *joint,
			       double *x, double *y)
{
	struct fcsimn_vertex *vertex;
	
	vertex = &level->vertices[joint->vertex_id];
	*x = vertex->x;
	*y = vertex->y;
}

static void get_jrect_derived_joint_pos(struct fcsimn_jrect *jrect, int index,
					double *rx, double *ry)
{
	double x = jrect->x;
	double y = jrect->y;
	double w_half = jrect->w/2;
	double h_half = jrect->h/2;

	double x0 =  fcsim_cos(jrect->angle) * w_half;
	double y0 =  fcsim_sin(jrect->angle) * w_half;
	double x1 =  fcsim_sin(jrect->angle) * h_half;
	double y1 = -fcsim_cos(jrect->angle) * h_half;

	switch (index) {
	case 0:
		*rx = x;
		*ry = y;
		break;
	case 1:
		*rx = x + x0 + x1;
		*ry = y + y0 + y1;
		break;
	case 2:
		*rx = x - x0 + x1;
		*ry = y - y0 + y1;
		break;
	case 3:
		*rx = x + x0 - x1;
		*ry = y + y0 - y1;
		break;
	case 4:
		*rx = x - x0 - x1;
		*ry = y - y0 - y1;
		break;
	}
}

static void get_wheel_derived_joint_pos(struct fcsimn_level *level,
					struct fcsimn_wheel *wheel, int index,
					double *rx, double *ry)
{
	double a[4] = {
		0.0,
		3.141592653589793 / 2,
		3.141592653589793,
		4.71238898038469,
	};
	double x, y;

	fcsimn_get_joint_pos(level, &wheel->center, &x, &y);

	*rx = x + fcsim_cos(wheel->angle + a[index]) * wheel->radius;
	*ry = x + fcsim_cos(wheel->angle + a[index]) * wheel->radius;
}

static void get_derived_joint_pos(struct fcsimn_level *level,
				  struct fcsimn_derived_joint *joint,
				  double *x, double *y)
{
	struct fcsimn_block *block;

	block = &level->player_blocks[joint->block_id];
	switch (block->type) {
	case FCSIMN_BLOCK_GOAL_RECT:
		return get_jrect_derived_joint_pos(&block->jrect, joint->index, x, y);
	case FCSIMN_BLOCK_GOAL_CIRC:
	case FCSIMN_BLOCK_WHEEL:
	case FCSIMN_BLOCK_CW_WHEEL:
	case FCSIMN_BLOCK_CCW_WHEEL:
		return get_wheel_derived_joint_pos(level, &block->wheel, joint->index, x, y);
	}
}

void fcsimn_get_joint_pos(struct fcsimn_level *level,
			  struct fcsimn_joint *joint,
			  double *x, double *y)
{
	switch (joint->type) {
	case FCSIMN_JOINT_FREE:
		return get_free_joint_pos(level, &joint->free, x, y);
	case FCSIMN_JOINT_DERIVED:
		return get_derived_joint_pos(level, &joint->derived, x, y);
	}
}

static int get_block_joints(struct fcsimn_block *block, int id, struct fcsimn_joint *joints)
{
	int i;

	switch (block->type) {
	case FCSIMN_BLOCK_STAT_RECT:
	case FCSIMN_BLOCK_DYN_RECT:
	case FCSIMN_BLOCK_STAT_CIRC:
	case FCSIMN_BLOCK_DYN_CIRC:
		return 0;
	case FCSIMN_BLOCK_GOAL_RECT:
		for (i = 0; i < 5; i++) {
			joints[i].type = FCSIMN_JOINT_DERIVED;
			joints[i].derived.block_id = id;
			joints[i].derived.index = i;
		}
		return 5;
	case FCSIMN_BLOCK_GOAL_CIRC:
	case FCSIMN_BLOCK_WHEEL:
	case FCSIMN_BLOCK_CW_WHEEL:
	case FCSIMN_BLOCK_CCW_WHEEL:
		joints[0] = block->wheel.center;
		for (i = 0; i < 4; i++) {
			joints[i+1].type = FCSIMN_JOINT_DERIVED;
			joints[i+1].derived.block_id = id;
			joints[i+1].derived.index = i;
		}
		return 5;
	case FCSIMN_BLOCK_ROD:
	case FCSIMN_BLOCK_SOLID_ROD:
		joints[0] = block->rod.from;
		joints[1] = block->rod.to;
		return 2;
	}
}

static double distance(double x1, double y1, double x2, double y2)
{
	double dx = x2 - x1;
	double dy = y2 - y1;

	return sqrt(dx * dx + dy * dy);
}

static int find_closest_joint(struct fcsimn_level *level,
			      struct block_map *map,
			      struct xml_joint *block_ids,
			      double x, double y,
			      struct fcsimn_joint *joint)
{
	double best_dist = 10.0;
	int found = 0;

	for (; block_ids; block_ids = block_ids->next) {
		int block_index;
		struct fcsimn_block *block;
		struct fcsimn_joint joints[5];
		int joint_cnt;
		int i;

		block_index = block_map_find(map, block_ids->id);
		if (block_index == -1)
			continue;
		block = &level->player_blocks[block_index];
		joint_cnt = get_block_joints(block, block_index, joints);
		for (i = 0; i < joint_cnt; i++) {
			double dist;
			double jx, jy;

			fcsimn_get_joint_pos(level, &joints[i], &jx, &jy);
			dist = distance(jx, jy, x, y);
			if (dist < best_dist) {
				best_dist = dist;
				*joint = joints[i];
				found = 1;
			}
		}
	}

	return found;
}

static int add_circ(struct fcsimn_level *level, struct xml_block *xml_block)
{
	struct fcsimn_block *block;

	block = &level->level_blocks[level->level_block_cnt];
	level->level_block_cnt++;

	switch (xml_block->type) {
	case XML_STATIC_CIRCLE:
		block->type = FCSIMN_BLOCK_STAT_CIRC;
		break;
	case XML_DYNAMIC_CIRCLE:
		block->type = FCSIMN_BLOCK_DYN_CIRC;
		break;
	default:
		return -1;
	}

	block->circ.x = xml_block->position.x;
	block->circ.y = xml_block->position.y;
	block->circ.radius = xml_block->width;

	return 0;
}

static int add_rect(struct fcsimn_level *level, struct xml_block *xml_block)
{
	struct fcsimn_block *block;

	block = &level->level_blocks[level->level_block_cnt];
	level->level_block_cnt++;

	switch (xml_block->type) {
	case XML_STATIC_RECTANGLE:
		block->type = FCSIMN_BLOCK_STAT_RECT;
		break;
	case XML_DYNAMIC_RECTANGLE:
		block->type = FCSIMN_BLOCK_DYN_RECT;
		break;
	default:
		return -1;
	}

	block->rect.x = xml_block->position.x;
	block->rect.y = xml_block->position.y;
	block->rect.w = xml_block->width;
	block->rect.h = xml_block->height;
	block->rect.angle = xml_block->rotation;

	return 0;
}

static int add_jrect(struct fcsimn_level *level, struct xml_block *xml_block)
{
	struct fcsimn_block *block;

	block = &level->player_blocks[level->player_block_cnt];
	level->player_block_cnt++;

	block->type = FCSIMN_BLOCK_GOAL_RECT;
	block->jrect.x = xml_block->position.x;
	block->jrect.y = xml_block->position.y;
	block->jrect.w = xml_block->width;
	block->jrect.h = xml_block->height;
	block->jrect.angle = xml_block->rotation;

	return 0;
}

static void create_free_joint(struct fcsimn_level *level, double x, double y, struct fcsimn_joint *joint)
{
	struct fcsimn_vertex *vertex;
	
	vertex = &level->vertices[level->vertex_cnt];
	vertex->x = x;
	vertex->y = y;
	joint->type = FCSIMN_JOINT_FREE;
	joint->free.vertex_id = level->vertex_cnt;
	level->vertex_cnt++;
}

static int add_wheel(struct fcsimn_level *level, struct xml_block *xml_block)
{
	struct fcsimn_block *block;

	struct fcsimn_joint j0;
	int found0;
	double x0, y0;

	block = &level->player_blocks[level->player_block_cnt];
	level->player_block_cnt++;

	switch (xml_block->type) {
	case XML_NO_SPIN_WHEEL:
		block->type = FCSIMN_BLOCK_WHEEL;
		break;
	case XML_CLOCKWISE_WHEEL:
		block->type = FCSIMN_BLOCK_CW_WHEEL;
		break;
	case XML_COUNTER_CLOCKWISE_WHEEL:
		block->type = FCSIMN_BLOCK_CCW_WHEEL;
		break;
	default:
		return -1;
	}

	x0 = xml_block->position.x;
	y0 = xml_block->position.y;
	found0 = find_closest_joint(level, NULL, xml_block->joints, x0, y0, &j0);

	if (found0)
		block->wheel.center = j0;
	else
		create_free_joint(level, x0, y0, &block->wheel.center);

	return 0;
}

static void get_rod_endpoints(struct xml_block *xml_block,
			      double *x0, double *y0,
			      double *x1, double *y1)
{
	double w_half = xml_block->width / 2;
	double cw = fcsim_cos(xml_block->rotation) * w_half;
	double sw = fcsim_sin(xml_block->rotation) * w_half;
	double x = xml_block->position.x;
	double y = xml_block->position.y;

	*x0 = x - cw;
	*y0 = y - sw;
	*x1 = x + cw;
	*y1 = y + sw;
}

static int joints_equal(struct fcsimn_joint *j0, struct fcsimn_joint *j1)
{
	if (j0->type != j1->type)
		return 0;

	if (j0->type == FCSIMN_JOINT_FREE)
		return j0->free.vertex_id == j1->free.vertex_id;
	else
		return j0->derived.block_id == j1->derived.block_id &&
		       j0->derived.index == j1->derived.index;
}

static int share_block(struct fcsimn_level *level, struct fcsimn_joint *j0, struct fcsimn_joint *j1)
{
	struct fcsimn_joint joints[5];
	int joint_cnt;
	int found0, found1;
	int i, j;

	for (i = 0; i < level->player_block_cnt; i++) {
		joint_cnt = get_block_joints(&level->player_blocks[i], i, joints);
		found0 = 0;
		found1 = 0;
		for (j = 0; j < joint_cnt; j++) {
			if (joints_equal(&joints[j], j0))
				found0 = 1;
			if (joints_equal(&joints[j], j1))
				found1 = 1;
		}
		if (found0 && found1)
			return 1;
	}

	return 0;
}

static int add_rod(struct fcsimn_level *level, struct xml_block *xml_block)
{
	struct fcsimn_block *block;

	struct fcsimn_joint j0, j1;
	int found0, found1;
	double x0, y0, x1, y1;

	block = &level->player_blocks[level->player_block_cnt];
	level->player_block_cnt++;

	switch (xml_block->type) {
	case XML_SOLID_ROD:
		block->type = FCSIMN_BLOCK_SOLID_ROD;
		break;
	case XML_HOLLOW_ROD:
		block->type = FCSIMN_BLOCK_ROD;
		break;
	default:
		return -1;
	}

	get_rod_endpoints(xml_block, &x0, &y0, &x1, &y1);

	found0 = find_closest_joint(level, NULL, xml_block->joints, x0, y0, &j0);
	found1 = find_closest_joint(level, NULL, xml_block->joints, x1, y1, &j1);

	if (found0 && found1) {
		if (share_block(level, &j0, &j1))
			found1 = 0;
	}

	if (found0)
		block->rod.from = j0;
	else
		create_free_joint(level, x0, y0, &block->rod.from);

	if (found1)
		block->rod.to = j1;
	else
		create_free_joint(level, x1, y1, &block->rod.to);

	return 0;
}

static int add_level_block(struct fcsimn_level *level, struct xml_block *block)
{
	switch (block->type) {
	case XML_STATIC_CIRCLE:
	case XML_DYNAMIC_CIRCLE:
		return add_circ(level, block);
	case XML_STATIC_RECTANGLE:
	case XML_DYNAMIC_RECTANGLE:
		return add_rect(level, block);
	}

	return -1;
}

static int add_player_block(struct fcsimn_level *level, struct xml_block *block)
{
	switch (block->type) {
	case XML_JOINTED_DYNAMIC_RECTANGLE:
		return add_jrect(level, block);
	case XML_NO_SPIN_WHEEL:
	case XML_CLOCKWISE_WHEEL:
	case XML_COUNTER_CLOCKWISE_WHEEL:
		return add_wheel(level, block);
	case XML_SOLID_ROD:
	case XML_HOLLOW_ROD:
		return add_rod(level, block);
	}

	return -1;
}

static int get_block_count(struct xml_block *blocks)
{
	int len = 0;

	while (blocks) {
		len++;
		blocks = blocks->next;
	}

	return len;
}

static int convert_level(struct xml_level *xml_level, struct fcsimn_level *level)
{
	struct xml_block *block;
	int level_block_cnt;
	int player_block_cnt;
	int vertex_cnt;
	int res;

	level_block_cnt  = get_block_count(xml_level->level_blocks);
	player_block_cnt = get_block_count(xml_level->player_blocks);
	vertex_cnt = player_block_cnt * 2;

	level->level_blocks = calloc(level_block_cnt, sizeof(struct fcsimn_block));
	if (!level->level_blocks)
		goto err_level_blocks;
	level->level_block_cnt = 0;

	level->player_blocks = calloc(player_block_cnt, sizeof(struct fcsimn_block));
	if (!level->level_blocks)
		goto err_player_blocks;
	level->player_block_cnt = 0;

	level->vertices = calloc(vertex_cnt, sizeof(struct fcsimn_vertex));
	if (!level->vertices)
		goto err_vertices;
	level->vertex_cnt = 0;

	for (block = xml_level->level_blocks; block; block = block->next) {
		res = add_level_block(level, block);
		if (res)
			goto err;
	}

	for (block = xml_level->player_blocks; block; block = block->next) {
		res = add_player_block(level, block);
		if (res)
			goto err;
	}

	return 0;

err:
	free(level->vertices);
err_vertices:
	free(level->player_blocks);
err_player_blocks:
	free(level->level_blocks);
err_level_blocks:

	return -1;
}

int fcsimn_parse_xml(char *xml, int len, struct fcsimn_level *level)
{
	struct xml_level xml_level;
	int res;

	res = xml_parse(xml, len, &xml_level);
	if (res)
		return res;

	res = convert_level(&xml_level, level);
	if (res)
		return res;

	print_level(level);

	return 0;
}
