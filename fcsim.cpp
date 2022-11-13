#include <vector>
#include <stdio.h>

#include "box2d/Include/Box2D.h"

#include "fcsim.h"

#define ARENA_WIDTH	2000
#define ARENA_HEIGHT	1450
#define TIME_STEP	0.03333333333333333
#define ITERATIONS	10

struct block_descr;

struct joint_collection {
	float x, y;
	int cnt;
	block_descr *top_block;
};

struct joint_descr {
	joint_collection *jc;
	block_descr *to_block;
	joint_descr *next;
};

struct block_descr {
	fcsim_block *block;
	b2Body *body;
	joint_descr *joints;
};

b2World *the_world;

joint_collection the_joints[1000];
block_descr      the_blocks[1000];
int the_joint_cnt;
int the_block_cnt;

static float distance(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;

	return sqrtf(dx * dx + dy * dy);
}

class fcsim_collision_filter : public b2CollisionFilter {
	bool ShouldCollide(b2Shape *s1, b2Shape *s2)
	{
		if (!b2_defaultFilter.ShouldCollide(s1, s2))
			return false;

		block_descr *b1 = (block_descr *)s1->GetUserData();
		block_descr *b2 = (block_descr *)s2->GetUserData();

		for (joint_descr *j1 = b1->joints; j1; j1 = j1->next) {
			for (joint_descr *j2 = b2->joints; j2; j2 = j2->next) {
				if (j1->jc == j2->jc)
					return false;
			}
		}

		return true;
	}
};

static fcsim_collision_filter the_collision_filter;

void fcsim_create_world(void)
{
	b2Vec2 gravity(0, 300);
	b2AABB aabb;
	aabb.minVertex.Set(-ARENA_WIDTH, -ARENA_HEIGHT);
	aabb.maxVertex.Set( ARENA_WIDTH,  ARENA_HEIGHT);

	the_world = new b2World(aabb, gravity, true);
	the_world->SetFilter(&the_collision_filter);
}

struct block_physics {
	int circle;
	float density;
	float friction;
	float restitution;
	int categoryBits;
	int maskBits;
	float linearDamping;
	float angularDamping;
};

static struct block_physics block_physics_tbl[] = {
	/* c  dens  fric  rest   cB   mB    linD  angD */
	{  0, 0.0f, 0.7f, 0.0f,  -1,  -1,   0.0f, 0.0f }, /* STAT_RECT */
	{  1, 0.0f, 0.7f, 0.0f,  -1,  -1,   0.0f, 0.0f }, /* STAT_CIRCLE */
	{  0, 1.0f, 0.7f, 0.2f,  -1,  -1,   0.0f, 0.0f }, /* DYN_RECT */
	{  1, 1.0f, 0.7f, 0.2f,  -1,  -1,   0.0f, 0.0f }, /* DYN_CIRCLE */
	{  0, 1.0f, 0.7f, 0.2f,   1, -17,   0.0f, 0.0f }, /* GOAL_RECT */
	{  1, 1.0f, 0.7f, 0.2f,   1, -17,   0.0f, 0.0f }, /* GOAL_CIRCLE */
	{  1, 1.0f, 0.7f, 0.2f,   1, -17,   0.0f, 0.0f }, /* WHEEL */
	{  1, 1.0f, 0.7f, 0.2f,   1, -17,   0.0f, 0.0f }, /* CW_WHEEL */
	{  1, 1.0f, 0.7f, 0.2f,   1, -17,   0.0f, 0.0f }, /* CCW_WHEEL */
	{  0, 1.0f, 0.7f, 0.2f,  16, -18, 0.009f, 0.2f }, /* ROD */
	{  0, 1.0f, 0.7f, 0.2f, 256, -17, 0.009f, 0.2f }, /* SOLID_ROD */
};

static void create_block_body(block_descr *bd)
{
	fcsim_block *block = bd->block;
	block_physics phys = block_physics_tbl[block->type];
	b2BoxDef box_def;
	b2CircleDef circle_def;
	b2ShapeDef *shape_def;
	b2BodyDef body_def;

	if (phys.circle) {
		circle_def.radius = block->w/2;
		shape_def = &circle_def;
	} else {
		box_def.extents.Set(block->w/2, block->h/2);
		shape_def = &box_def;
	}
	shape_def->density = phys.density;
	shape_def->friction = phys.friction;
	shape_def->restitution = phys.restitution;
	shape_def->categoryBits = phys.categoryBits;
	shape_def->maskBits = phys.maskBits;
	shape_def->userData = bd;
	body_def.position.Set(block->x, block->y);
	body_def.rotation = block->angle;
	body_def.linearDamping = phys.linearDamping;
	body_def.angularDamping = phys.angularDamping;
	body_def.AddShape(shape_def);
	bd->body = the_world->CreateBody(&body_def);
}

static joint_collection *get_closest_jc(float x, float y, int joints[2])
{
	float best_dist = 10.0f;
	joint_collection *res = NULL;

	for (int i = 0; i < 2; i++) {
		int block_idx = joints[i];
		if (block_idx < 0)
			continue;
		block_descr *bd = &the_blocks[block_idx];
		for (joint_descr *j = bd->joints; j; j = j->next) {
			float dist = distance(x, y, j->jc->x, j->jc->y);
			if (dist < best_dist) {
				best_dist = dist;
				res = j->jc;
			}
		}
	}

	return res;
}

void get_rod_endpoints(fcsim_block *block, float *x0, float *y0, float *x1, float *y1)
{
	float cw = cosf(block->angle) * block->w / 2;
	float sw = sinf(block->angle) * block->w / 2;

	*x0 = block->x - cw;
	*y0 = block->y - sw;
	*x1 = block->x + cw;
	*y1 = block->y + sw;
}

int share_block(joint_collection *jc0, joint_collection *jc1)
{
	for (int i = 0; i < the_block_cnt; i++) {
		bool f0 = false, f1 = false;
		for (joint_descr *j = the_blocks[i].joints; j; j = j->next) {
			if (j->jc == jc0) f0 = true;
			if (j->jc == jc1) f1 = true;
		}
		if (f0 && f1) return true;
	}
	return false;
}

#define FCSIM_JOINT_PIN 0
#define FCSIM_JOINT_CW  1
#define FCSIM_JOINT_CCW -1

static int joint_type(int block_type)
{
	switch (block_type) {
		case FCSIM_CW_WHEEL: return FCSIM_JOINT_CW;
		case FCSIM_CCW_WHEEL: return FCSIM_JOINT_CCW;
	}
	return FCSIM_JOINT_PIN;
}

static int is_wheel(int block_type)
{
	switch (block_type) {
	case FCSIM_GOAL_CIRCLE:
	case FCSIM_WHEEL:
	case FCSIM_CW_WHEEL:
	case FCSIM_CCW_WHEEL:
		return true;
	}
	return false;
}

static void create_joint(b2Body *b1, b2Body *b2, float x, float y, int type)
{
	b2RevoluteJointDef joint_def;

	joint_def.body1 = b1;
	joint_def.body2 = b2;
	joint_def.anchorPoint.Set(x, y);
	joint_def.collideConnected = true;
	if (type != FCSIM_JOINT_PIN) {
		joint_def.motorTorque = 50000000;
		joint_def.motorSpeed = -5 * type;
		joint_def.enableMotor = true;
	}
	the_world->CreateJoint(&joint_def);
}

static void create_block_joint(block_descr *bd, joint_descr *j)
{
	joint_collection *jc = j->jc;
	block_descr *to = j->to_block;
	if (!to)
		return;

	int type = joint_type(bd->block->type);
	if (jc->cnt == 2 && is_wheel(to->block->type))
		type = joint_type(to->block->type);
	if (type != FCSIM_JOINT_PIN && is_wheel(bd->block->type))
		type = -type;

	create_joint(to->body, bd->body, jc->x, jc->y, type);
}

static void create_block_joints(block_descr *bd)
{
	for (joint_descr *j = bd->joints; j; j = j->next)
		create_block_joint(bd, j);
}

static void connect_joint(block_descr *bd, joint_collection *jc)
{
	joint_descr *j = new joint_descr;
	j->jc = jc;
	j->to_block = jc->top_block;
	j->next = bd->joints;
	jc->top_block = bd;
	jc->cnt++;
	bd->joints = j;
}

static void make_block_joint(block_descr *bd, float x, float y)
{
	joint_collection *jc = &the_joints[the_joint_cnt++];

	jc->x = x;
	jc->y = y;
	jc->cnt = 0;
	jc->top_block = NULL;

	connect_joint(bd, jc);
}

static void make_connecting_block_joint(block_descr *bd, joint_collection *jc, float *x, float *y)
{
	if (jc) {
		*x = jc->x;
		*y = jc->y;
		connect_joint(bd, jc);
	} else {
		make_block_joint(bd, *x, *y);
	}
}

static void do_rod_joints(struct block_descr *bd)
{
	fcsim_block *block = bd->block;
	float x0, y0, x1, y1;

	get_rod_endpoints(block, &x0, &y0, &x1, &y1);
	joint_collection *j0 = get_closest_jc(x0, y0, block->joints);
	joint_collection *j1 = get_closest_jc(x1, y1, block->joints);

	if (j0 && j1 && share_block(j0, j1))
		j1 = NULL;

	make_connecting_block_joint(bd, j0, &x0, &y0);
	make_connecting_block_joint(bd, j1, &x1, &y1);

	/* todo: update block position & rotation from x0, y0, x1, y1 */
}
		
static void do_wheel_joints(struct block_descr *bd)
{
	fcsim_block *block = bd->block;
	float x = block->x;
	float y = block->y;
	float r = block->w/2;
	joint_collection *j = get_closest_jc(x, y, block->joints);

	make_connecting_block_joint(bd, j, &x, &y);
	block->x = x;
	block->y = y;
	make_block_joint(bd, x + r, y);
	make_block_joint(bd, x - r, y);
	make_block_joint(bd, x, y + r);
	make_block_joint(bd, x, y - r);
}

static void do_goal_rect_joints(struct block_descr *bd)
{
	fcsim_block *block = bd->block;
	float x = block->x;
	float y = block->y;
	float w = block->w/2;
	float h = block->h/2;

	/* todo: take rotation into account */
	make_block_joint(bd, x, y);
	make_block_joint(bd, x + w, y + w);
	make_block_joint(bd, x + w, y - w);
	make_block_joint(bd, x - w, y + w);
	make_block_joint(bd, x - w, y - w);
}

static void do_joints(struct block_descr *bd)
{
	switch (bd->block->type) {
	case FCSIM_GOAL_RECT:
		do_goal_rect_joints(bd);
		return;
	case FCSIM_GOAL_CIRCLE:
	case FCSIM_WHEEL:
	case FCSIM_CW_WHEEL:
	case FCSIM_CCW_WHEEL:
		do_wheel_joints(bd);
		return;
	case FCSIM_ROD:
	case FCSIM_SOLID_ROD:
		do_rod_joints(bd);
		return;
	}
}

void fcsim_add_block(struct fcsim_block *block)
{
	block_descr *bd = &the_blocks[the_block_cnt++];
	bd->block = block;

	assert(block->type >= 0 && block->type <= FCSIM_TYPE_MAX);
	assert(block->joints[0] < the_block_cnt);
	assert(block->joints[1] < the_block_cnt);

	if (block->type == FCSIM_STAT_CIRCLE || block->type == FCSIM_DYN_CIRCLE) {
		block->w *= 2;
		block->h *= 2;
	}

	do_joints(bd);
	create_block_body(bd);
	create_block_joints(bd);
}

void fcsim_step(void)
{
	the_world->Step(TIME_STEP, ITERATIONS);

	for (int i = 0; i < the_block_cnt; i++) {
		block_descr *bd = &the_blocks[i];
		b2Vec2 pos  = bd->body->GetOriginPosition();
		float angle = bd->body->GetRotation();

		bd->block->x = pos.x;
		bd->block->y = pos.y;
		bd->block->angle = angle;
	}
}
