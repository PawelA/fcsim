/*
#include <stdlib.h>
#include <math.h>
#include <fpmath/fpmath.h>

#include "xml.h"
#include "fcsim.h"

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

static void get_free_joint_pos(struct fcsim_level *level,
			       struct fcsim_free_joint *joint,
			       double *x, double *y)
{
	struct fcsim_vertex *vertex;
	
	vertex = &level->vertices[joint->vertex_id];
	*x = vertex->x;
	*y = vertex->y;
}

static void get_jrect_derived_joint_pos(struct fcsim_jrect *jrect, int index,
					double *rx, double *ry)
{
	double x = jrect->x;
	double y = jrect->y;
	double w_half = jrect->w/2;
	double h_half = jrect->h/2;

	double x0 =  fp_cos(jrect->angle) * w_half;
	double y0 =  fp_sin(jrect->angle) * w_half;
	double x1 =  fp_sin(jrect->angle) * h_half;
	double y1 = -fp_cos(jrect->angle) * h_half;

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

static void get_wheel_derived_joint_pos(struct fcsim_level *level,
					struct fcsim_wheel *wheel, int index,
					double *rx, double *ry)
{
	double a[4] = {
		0.0,
		3.141592653589793 / 2,
		3.141592653589793,
		4.71238898038469,
	};
	double x, y;

	fcsim_get_joint_pos(level, &wheel->center, &x, &y);

	*rx = x + fp_cos(wheel->angle + a[index]) * wheel->radius;
	*ry = y + fp_sin(wheel->angle + a[index]) * wheel->radius;
}

static void get_derived_joint_pos(struct fcsim_level *level,
				  struct fcsim_derived_joint *joint,
				  double *x, double *y)
{
	struct fcsim_block *block;

	block = &level->player_blocks[joint->block_id];
	switch (block->type) {
	case FCSIM_BLOCK_GOAL_RECT:
		return get_jrect_derived_joint_pos(&block->jrect, joint->index, x, y);
	case FCSIM_BLOCK_GOAL_CIRC:
	case FCSIM_BLOCK_WHEEL:
	case FCSIM_BLOCK_CW_WHEEL:
	case FCSIM_BLOCK_CCW_WHEEL:
		return get_wheel_derived_joint_pos(level, &block->wheel, joint->index, x, y);
	}
}

void fcsim_get_joint_pos(struct fcsim_level *level,
			  struct fcsim_joint *joint,
			  double *x, double *y)
{
	switch (joint->type) {
	case FCSIM_JOINT_FREE:
		return get_free_joint_pos(level, &joint->free, x, y);
	case FCSIM_JOINT_DERIVED:
		return get_derived_joint_pos(level, &joint->derived, x, y);
	}
}

int fcsim_get_player_block_joints(struct fcsim_level *level, int id, struct fcsim_joint *joints)
{
	struct fcsim_block *block = &level->player_blocks[id];
	int i;

	switch (block->type) {
	case FCSIM_BLOCK_STAT_RECT:
	case FCSIM_BLOCK_DYN_RECT:
	case FCSIM_BLOCK_STAT_CIRC:
	case FCSIM_BLOCK_DYN_CIRC:
		return 0;
	case FCSIM_BLOCK_GOAL_RECT:
		for (i = 0; i < 5; i++) {
			joints[i].type = FCSIM_JOINT_DERIVED;
			joints[i].derived.block_id = id;
			joints[i].derived.index = i;
		}
		return 5;
	case FCSIM_BLOCK_GOAL_CIRC:
	case FCSIM_BLOCK_WHEEL:
	case FCSIM_BLOCK_CW_WHEEL:
	case FCSIM_BLOCK_CCW_WHEEL:
		joints[0] = block->wheel.center;
		for (i = 0; i < 4; i++) {
			joints[i+1].type = FCSIM_JOINT_DERIVED;
			joints[i+1].derived.block_id = id;
			joints[i+1].derived.index = i;
		}
		return 5;
	case FCSIM_BLOCK_ROD:
	case FCSIM_BLOCK_SOLID_ROD:
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

static int find_closest_joint(struct fcsim_level *level,
			      struct block_map *map,
			      struct xml_joint *block_ids,
			      double x, double y,
			      struct fcsim_joint *joint)
{
	double best_dist = 10.0;
	int found = 0;

	for (; block_ids; block_ids = block_ids->next) {
		int block_index;
		struct fcsim_block *block;
		struct fcsim_joint joints[5];
		int joint_cnt;
		int i;

		block_index = block_map_find(map, block_ids->id);
		if (block_index == -1)
			continue;
		block = &level->player_blocks[block_index];
		joint_cnt = fcsim_get_player_block_joints(level, block_index, joints);
		for (i = 0; i < joint_cnt; i++) {
			double dist;
			double jx, jy;

			fcsim_get_joint_pos(level, &joints[i], &jx, &jy);
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

static int add_circ(struct fcsim_level *level, struct xml_block *xml_block)
{
	struct fcsim_block *block;

	block = &level->level_blocks[level->level_block_cnt];
	level->level_block_cnt++;

	switch (xml_block->type) {
	case XML_STATIC_CIRCLE:
		block->type = FCSIM_BLOCK_STAT_CIRC;
		break;
	case XML_DYNAMIC_CIRCLE:
		block->type = FCSIM_BLOCK_DYN_CIRC;
		break;
	default:
		return -1;
	}

	block->circ.x = xml_block->position.x;
	block->circ.y = xml_block->position.y;
	block->circ.radius = xml_block->width;

	return 0;
}

static int add_rect(struct fcsim_level *level, struct xml_block *xml_block)
{
	struct fcsim_block *block;

	block = &level->level_blocks[level->level_block_cnt];
	level->level_block_cnt++;

	switch (xml_block->type) {
	case XML_STATIC_RECTANGLE:
		block->type = FCSIM_BLOCK_STAT_RECT;
		break;
	case XML_DYNAMIC_RECTANGLE:
		block->type = FCSIM_BLOCK_DYN_RECT;
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

static int add_jrect(struct fcsim_level *level, struct xml_block *xml_block)
{
	struct fcsim_block *block;

	block = &level->player_blocks[level->player_block_cnt];
	level->player_block_cnt++;

	block->type = FCSIM_BLOCK_GOAL_RECT;
	block->jrect.x = xml_block->position.x;
	block->jrect.y = xml_block->position.y;
	block->jrect.w = xml_block->width;
	block->jrect.h = xml_block->height;
	block->jrect.angle = xml_block->rotation;

	return 0;
}

static void create_free_joint(struct fcsim_level *level, double x, double y, struct fcsim_joint *joint)
{
	struct fcsim_vertex *vertex;
	
	vertex = &level->vertices[level->vertex_cnt];
	vertex->x = x;
	vertex->y = y;
	joint->type = FCSIM_JOINT_FREE;
	joint->free.vertex_id = level->vertex_cnt;
	level->vertex_cnt++;
}

static int add_wheel(struct fcsim_level *level, struct xml_block *xml_block)
{
	struct fcsim_block *block;

	struct fcsim_joint j0;
	int found0;
	double x0, y0;

	block = &level->player_blocks[level->player_block_cnt];
	level->player_block_cnt++;

	switch (xml_block->type) {
	case XML_NO_SPIN_WHEEL:
		if (xml_block->goal_block)
			block->type = FCSIM_BLOCK_GOAL_CIRC;
		else
			block->type = FCSIM_BLOCK_WHEEL;
		break;
	case XML_CLOCKWISE_WHEEL:
		block->type = FCSIM_BLOCK_CW_WHEEL;
		break;
	case XML_COUNTER_CLOCKWISE_WHEEL:
		block->type = FCSIM_BLOCK_CCW_WHEEL;
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

	block->wheel.radius = xml_block->width / 2;
	block->wheel.angle = xml_block->rotation;

	return 0;
}

static void get_rod_endpoints(struct xml_block *xml_block,
			      double *x0, double *y0,
			      double *x1, double *y1)
{
	double w_half = xml_block->width / 2;
	double cw = fp_cos(xml_block->rotation) * w_half;
	double sw = fp_sin(xml_block->rotation) * w_half;
	double x = xml_block->position.x;
	double y = xml_block->position.y;

	*x0 = x - cw;
	*y0 = y - sw;
	*x1 = x + cw;
	*y1 = y + sw;
}

static int joints_equal(struct fcsim_joint *j0, struct fcsim_joint *j1)
{
	if (j0->type != j1->type)
		return 0;

	if (j0->type == FCSIM_JOINT_FREE)
		return j0->free.vertex_id == j1->free.vertex_id;
	else
		return j0->derived.block_id == j1->derived.block_id &&
		       j0->derived.index == j1->derived.index;
}

static int share_block(struct fcsim_level *level, struct fcsim_joint *j0, struct fcsim_joint *j1)
{
	struct fcsim_joint joints[5];
	int joint_cnt;
	int found0, found1;
	int i, j;

	for (i = 0; i < level->player_block_cnt; i++) {
		joint_cnt = fcsim_get_player_block_joints(level, i, joints);
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

static int add_rod(struct fcsim_level *level, struct xml_block *xml_block)
{
	struct fcsim_block *block;

	struct fcsim_joint j0, j1;
	int found0, found1;
	double x0, y0, x1, y1;

	block = &level->player_blocks[level->player_block_cnt];
	level->player_block_cnt++;

	switch (xml_block->type) {
	case XML_SOLID_ROD:
		block->type = FCSIM_BLOCK_SOLID_ROD;
		break;
	case XML_HOLLOW_ROD:
		block->type = FCSIM_BLOCK_ROD;
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

static int add_level_block(struct fcsim_level *level, struct xml_block *block)
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

static int add_player_block(struct fcsim_level *level, struct xml_block *block)
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

static void convert_area(struct xml_zone *zone, struct fcsim_area *area)
{
	area->x = zone->position.x;
	area->y = zone->position.y;
	area->w = zone->width;
	area->h = zone->height;
}

static int convert_level(struct xml_level *xml_level, struct fcsim_level *level)
{
	struct xml_block *block;
	int level_block_cnt;
	int player_block_cnt;
	int vertex_cnt;
	int res;

	level_block_cnt  = get_block_count(xml_level->level_blocks);
	player_block_cnt = get_block_count(xml_level->player_blocks);
	vertex_cnt = player_block_cnt * 2;

	level->level_blocks = calloc(level_block_cnt, sizeof(struct fcsim_block));
	if (!level->level_blocks)
		goto err_level_blocks;
	level->level_block_cnt = 0;

	level->player_blocks = calloc(player_block_cnt, sizeof(struct fcsim_block));
	if (!level->level_blocks)
		goto err_player_blocks;
	level->player_block_cnt = 0;

	level->vertices = calloc(vertex_cnt, sizeof(struct fcsim_vertex));
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

	convert_area(&xml_level->start, &level->build_area);
	convert_area(&xml_level->end, &level->goal_area);

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

int fcsim_parse_xml(char *xml, int len, struct fcsim_level *level)
{
	struct xml_level xml_level;
	int res;

	res = xml_parse(xml, len, &xml_level);
	if (res)
		return res;

	res = convert_level(&xml_level, level);
	if (res)
		return res;

	return 0;
}
*/

static struct material static_env_material = {
	.density = 0.0,
	.friction = 0.7,
	.restitution = 0.0,
	.linear_damping = 0.0,
	.angular_damping = 0.0,
	.collision_bit = ENV_COLLISION_BIT,
	.collision_mask = ENV_COLLISION_MASK,
};

static struct material dynamic_env_material = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.linear_damping = 0.0,
	.angular_damping = 0.0,
	.collision_bit = ENV_COLLISION_BIT,
	.collision_mask = ENV_COLLISION_MASK,
};

static struct material solid_material = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.linear_damping = 0.0,
	.angular_damping = 0.0,
	.collision_bit = SOLID_COLLISION_BIT,
	.collision_mask = SOLID_COLLISION_MASK,
};

static struct material solid_rod_material = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.linear_damping = 0.009,
	.angular_damping = 0.2,
	.collision_bit = SOLID_COLLISION_BIT,
	.collision_mask = SOLID_COLLISION_MASK,
};

static struct material water_rod_material = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.linear_damping = 0.009,
	.angular_damping = 0.2,
	.collision_bit = WATER_COLLISION_BIT,
	.collision_mask = WATER_COLLISION_MASK,
};

static void append_block(struct block_list *list, struct block *block)
{
	if (list->tail) {
		list->tail->next = block;
		block->prev = list->tail;
		list->tail = block;
	} else {
		list->head = block;
		list->tail = block;
	}
}

static void add_rect(struct design *design, struct xml_block *xml_block, struct material *mat)
{
	struct block *block = malloc(sizeof(*block));

	block->prev = NULL;
	block->next = NULL;
	block->shape.type = SHAPE_RECT;
	block->shape.rect.x = xml_block->position.x;
	block->shape.rect.y = xml_block->position.y;
	block->shape.rect.w = xml_block->width;
	block->shape.rect.h = xml_block->height;
	block->shape.rect.angle = xml_block->rotation;

	append_block(&deisgn->level_blocks, block);
}

static void add_circ(struct design *design, struct xml_block *xml_block, struct material *mat)
{

}

static void add_box(struct design *design, struct xml_block *xml_block, struct material *mat)
{

}

static void add_rod(struct design *design, struct xml_block *xml_block, struct material *mat)
{

}

static void add_wheel(struct design *design, struct xml_block *xml_block, struct material *mat)
{

}

static void add_level_block(struct design *design, struct xml_block *block)
{
	
	switch (block->type) {
	case XML_STATIC_RECTANGLE:
		add_rect(design, block, &static_env_material);
		break;
	case XML_DYNAMIC_RECTANGLE:
		add_rect(design, block, &dynamic_env_material);
		break;
	case XML_STATIC_CIRCLE:
		add_circ(design, block, &static_env_material);
		break;
	case XML_DYNAMIC_CIRCLE:
		add_circ(design, block, &dynamic_env_material);
		break;
	}
}

static int add_player_block(struct design *design, struct xml_block *block)
{
	switch (block->type) {
	case XML_JOINTED_DYNAMIC_RECTANGLE:
		add_box(design, block, &solid_material);
		break;
	case XML_SOLID_ROD:
		add_rod(level, block, &solid_rod_material);
		break;
	case XML_HOLLOW_ROD:
		add_rod(level, block, &water_rod_material);
		break;
	case XML_NO_SPIN_WHEEL:
	case XML_CLOCKWISE_WHEEL:
	case XML_COUNTER_CLOCKWISE_WHEEL:
		add_wheel(level, block);
		break;
	}
}

void convert_xml(struct xml_level *xml_level, struct design *design)
{
	struct xml_block *block;

	design->level_blocks.head = NULL;
	design->level_blocks.tail = NULL;
	design->player_blocks.head = NULL;
	design->player_blocks.tail = NULL;

	for (block = xml_level->level_blocks; block; block = block->next)
		add_level_block(design, block);

	for (block = xml_level->player_blocks; block; block = block->next)
		add_player_block(level, block);
}
