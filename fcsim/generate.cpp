#include <fcsimn.h>
#include "box2d/Include/Box2D.h"

struct fcsimn_simul {
	b2World world;
};

static void init_b2world(b2World *world)
{
	b2Vec2 gravity;
	b2AABB aabb;

	gravity.Set(0, 300);
	aabb.minVertex.Set(-2000, -1450);
	aabb.maxVertex.Set(2000, 1450);
	new (world) b2World(aabb, gravity, true);
}

enum shape_type {
	SHAPE_CIRC,
	SHAPE_RECT,
};

struct shape {
	enum shape_type type;
	double x, y;
	union {
		struct {
			double w, h;
		};
		double radius;
	};
	double angle;
};

struct block_physics {
	double density;
	double friction;
	double restitution;
	int category_bits;
	int mask_bits;
	double linear_damping;
	double angular_damping;
};

static struct block_physics stat_phys = {
	.density = 0.0,
	.friction = 0.7,
	.restitution = 0.0,
	.category_bits = -1,
	.mask_bits = -1,
	.linear_damping = 0.0,
	.angular_damping = 0.0,
};

static struct block_physics dyn_phys = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.category_bits = -1,
	.mask_bits = -1,
	.linear_damping = 0.0,
	.angular_damping = 0.0,
};

static struct block_physics player_phys = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.category_bits = 1,
	.mask_bits = -17,
	.linear_damping = 0.0,
	.angular_damping = 0.0,
};

static struct block_physics rod_phys = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.category_bits = 16,
	.mask_bits = -18,
	.linear_damping = 0.009,
	.angular_damping = 0.2,
};

static const struct block_physics solid_rod_phys = {
	.density = 1.0,
	.friction = 0.7,
	.restitution = 0.2,
	.category_bits = 256,
	.mask_bits = -17,
	.linear_damping = 0.009,
	.angular_damping = 0.2,
};

static b2Body *generate_body(b2World *world, struct shape *shape, struct block_physics *phys)
{
	b2BoxDef box_def;
	b2CircleDef circle_def;
	b2ShapeDef *shape_def;
	b2BodyDef body_def;

	if (shape->type == SHAPE_CIRC) {
		circle_def.radius = shape->radius;
		shape_def = &circle_def;
	} else {
		box_def.extents.Set(shape->w/2, shape->h/2);
		shape_def = &box_def;
	}
	shape_def->density = phys->density;
	shape_def->friction = phys->friction;
	shape_def->restitution = phys->restitution;
	shape_def->categoryBits = phys->category_bits;
	shape_def->maskBits = phys->mask_bits;
	shape_def->userData = NULL;
	body_def.position.Set(shape->x, shape->y);
	body_def.rotation = shape->angle;
	body_def.linearDamping = phys->linear_damping;
	body_def.angularDamping = phys->angular_damping;
	body_def.AddShape(shape_def);

	return world->CreateBody(&body_def);
}

static void get_rect_shape(struct fcsimn_rect *rect, struct shape *shape)
{
	shape->type = SHAPE_RECT;
	shape->x = rect->x;
	shape->y = rect->y;
	shape->w = rect->w;
	shape->h = rect->h;
	shape->angle = rect->angle;
}

static void get_circ_shape(struct fcsimn_circ *circ, struct shape *shape)
{
	shape->type = SHAPE_CIRC;
	shape->x = circ->x;
	shape->y = circ->y;
	shape->radius = circ->radius;
	shape->angle = 0.0;
}

static double distance(double x1, double y1, double x2, double y2)
{
	double dx = x2 - x1;
	double dy = y2 - y1;

	return sqrt(dx * dx + dy * dy);
}

static void get_jrect_shape(struct fcsimn_jrect *jrect, struct shape *shape)
{
	shape->type = SHAPE_RECT;
	shape->x = jrect->x;
	shape->y = jrect->y;
	shape->w = jrect->w;
	shape->h = jrect->h;
	shape->angle = jrect->angle;
}

static void get_wheel_shape(struct fcsimn_level *level, struct fcsimn_wheel *wheel, struct shape *shape)
{
	double x, y;

	fcsimn_get_joint_pos(level, &wheel->center, &x, &y);

	shape->type = SHAPE_CIRC;
	shape->x = x;
	shape->y = y;
	shape->radius = wheel->radius;
	shape->angle = wheel->angle;
}

static void get_rod_shape(struct fcsimn_level *level, struct fcsimn_rod *rod, int solid, struct shape *shape)
{
	double x0, y0, x1, y1;

	fcsimn_get_joint_pos(level, &rod->from, &x0, &y0);
	fcsimn_get_joint_pos(level, &rod->to, &x1, &y1);

	shape->type = SHAPE_RECT;
	shape->x = x0 + (x1 - x0) / 2.0;
	shape->y = y0 + (y1 - y0) / 2.0;
	shape->w = distance(x0, y0, x1, y1);
	shape->h = solid ? 8.0 : 4.0;
	shape->angle = fcsim_atan2(y1 - y0, x1 - x0);
}

static void get_block_shape(struct fcsimn_level *level, struct fcsimn_block *block, struct shape *shape)
{
	switch (block->type) {
	case FCSIMN_BLOCK_STAT_RECT:
	case FCSIMN_BLOCK_DYN_RECT:
		get_rect_shape(&block->rect, shape);
		break;
	case FCSIMN_BLOCK_STAT_CIRC:
	case FCSIMN_BLOCK_DYN_CIRC:
		get_circ_shape(&block->circ, shape);
		break;
	case FCSIMN_BLOCK_GOAL_RECT:
		get_jrect_shape(&block->jrect, shape);
		break;
	case FCSIMN_BLOCK_GOAL_CIRC:
	case FCSIMN_BLOCK_WHEEL:
	case FCSIMN_BLOCK_CW_WHEEL:
	case FCSIMN_BLOCK_CCW_WHEEL:
		get_wheel_shape(level, &block->wheel, shape);
		break;
	case FCSIMN_BLOCK_ROD:
		get_rod_shape(level, &block->rod, 0, shape);
		break;
	case FCSIMN_BLOCK_SOLID_ROD:
		get_rod_shape(level, &block->rod, 1, shape);
		break;
	}
}

static void get_block_phys(struct fcsimn_block *block, struct block_physics *phys)
{
	switch (block->type) {
	case FCSIMN_BLOCK_STAT_RECT:
	case FCSIMN_BLOCK_STAT_CIRC:
		*phys = stat_phys;
		break;
	case FCSIMN_BLOCK_DYN_RECT:
	case FCSIMN_BLOCK_DYN_CIRC:
		*phys = dyn_phys;
		break;
	case FCSIMN_BLOCK_GOAL_RECT:
	case FCSIMN_BLOCK_GOAL_CIRC:
	case FCSIMN_BLOCK_WHEEL:
	case FCSIMN_BLOCK_CW_WHEEL:
	case FCSIMN_BLOCK_CCW_WHEEL:
		*phys = player_phys;
		break;
	case FCSIMN_BLOCK_ROD:
		*phys = rod_phys;
		break;
	case FCSIMN_BLOCK_SOLID_ROD:
		*phys = solid_rod_phys;
		break;
	}
}

static void generate_block_body(struct fcsimn_simul *simul, struct fcsimn_level *level, struct fcsimn_block *block)
{
	struct shape shape;
	struct block_physics phys;

	get_block_shape(level, block, &shape);
	get_block_phys(block, &phys);

	block->body = generate_body(&simul->world, &shape, &stat_phys);
}

/*
static void add_player_block(struct fcsim_simul *simul,
			     struct joint_map *map,
			     struct fcsimn_block *block, int id)
{

}
*/

struct body_list {
	b2Body *body;
	struct body_list *next;
};

struct joint_map_entry {
	struct fcsimn_joint joint;
	int generated;
	struct body_list *body_head;
	struct body_list *body_tail;
	struct joint_map_entry *next;
};

struct joint_map {
	struct joint_map_entry *head;
	struct joint_map_entry *tail;
};

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

void body_list_append(struct joint_map_entry *ent, b2Body *body)
{
	struct body_list *bl;

	bl = (struct body_list *)calloc(1, sizeof(*bl));
	bl->body = body;

	if (ent->body_tail)
		ent->body_tail->next = bl;
	else
		ent->body_head = bl;

	ent->body_tail = bl;
}

struct joint_map_entry *joint_map_append(struct joint_map *map, struct fcsimn_joint *joint)
{
	struct joint_map_entry *ent;

	ent = (struct joint_map_entry *)calloc(1, sizeof(*ent));
	ent->joint = *joint;

	if (map->tail)
		map->tail->next = ent;
	else
		map->head = ent;

	map->tail = ent;

	return ent;
}

void joint_map_add(struct joint_map *map, struct fcsimn_joint *joint, b2Body *body)
{
	struct joint_map_entry *ent;

	for (ent = map->head; ent; ent = ent->next) {
		if (joints_equal(&ent->joint, joint)) {
			body_list_append(ent, body);
			return;
		}
	}

	ent = joint_map_append(map, joint);
	body_list_append(ent, body);
}

struct joint_map_entry *joint_map_find(struct joint_map *map, struct fcsimn_joint *joint)
{
	struct joint_map_entry *ent;

	for (ent = map->head; ent; ent = ent->next) {
		if (joints_equal(&ent->joint, joint))
			return ent;
	}

	return NULL;
}

struct fcsimn_simul *fcsimn_make_simul(struct fcsimn_level *level)
{
	struct fcsimn_simul *simul;
	int i;

	simul = (struct fcsimn_simul *)malloc(sizeof(*simul));
	if (!simul)
		return NULL;

	init_b2world(&simul->world);

	for (i = 0; i < level->level_block_cnt; i++)
		generate_block_body(simul, level, &level->level_blocks[i]);

	return simul;
}

void fcsimn_step(struct fcsimn_simul *simul)
{
	simul->world.Step(1.0 / 30.0, 10);

	b2Joint *joint = simul->world.GetJointList();
	while (joint) {
		b2Joint *next = joint->GetNext();
		b2Vec2 a1 = joint->GetAnchor1();
		b2Vec2 a2 = joint->GetAnchor2();
		b2Vec2 d = a1 - a2;
		if (fabs(d.x) + fabs(d.y) > 50.0)
			simul->world.DestroyJoint(joint);
		joint = next;
	}
}

int fcsimn_get(struct fcsimn_block *block, double *x, double *y, double *angle)
{
	b2Body *body = (b2Body *)block->body;
	b2Vec2 pos;

	if (!body)
		return -1;

	pos = body->GetOriginPosition();
	*x = pos.x;
	*y = pos.y;
	*angle = body->GetRotation(); 

	return 0;
}
