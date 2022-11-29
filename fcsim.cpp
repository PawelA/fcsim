#include "box2d/Include/Box2D.h"
#include "fcsim.h"
#include "net.h"

#define ARENA_WIDTH	2000
#define ARENA_HEIGHT	1450
#define TIME_STEP	0.03333333333333333
#define ITERATIONS	10

struct block_descr;

struct joint_collection {
	double x, y;
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

static double distance(double x1, double y1, double x2, double y2)
{
	double dx = x2 - x1;
	double dy = y2 - y1;

	return sqrt(dx * dx + dy * dy);
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
	double density;
	double friction;
	double restitution;
	int categoryBits;
	int maskBits;
	double linearDamping;
	double angularDamping;
};

static struct block_physics block_physics_tbl[] = {
	/* c  dens  fric  rest   cB   mB    linD  angD */
	{  0, 0.0, 0.7, 0.0,  -1,  -1,   0.0, 0.0 }, /* STAT_RECT */
	{  1, 0.0, 0.7, 0.0,  -1,  -1,   0.0, 0.0 }, /* STAT_CIRCLE */
	{  0, 1.0, 0.7, 0.2,  -1,  -1,   0.0, 0.0 }, /* DYN_RECT */
	{  1, 1.0, 0.7, 0.2,  -1,  -1,   0.0, 0.0 }, /* DYN_CIRCLE */
	{  0, 1.0, 0.7, 0.2,   1, -17,   0.0, 0.0 }, /* GOAL_RECT */
	{  1, 1.0, 0.7, 0.2,   1, -17,   0.0, 0.0 }, /* GOAL_CIRCLE */
	{  1, 1.0, 0.7, 0.2,   1, -17,   0.0, 0.0 }, /* WHEEL */
	{  1, 1.0, 0.7, 0.2,   1, -17,   0.0, 0.0 }, /* CW_WHEEL */
	{  1, 1.0, 0.7, 0.2,   1, -17,   0.0, 0.0 }, /* CCW_WHEEL */
	{  0, 1.0, 0.7, 0.2,  16, -18, 0.009, 0.2 }, /* ROD */
	{  0, 1.0, 0.7, 0.2, 256, -17, 0.009, 0.2 }, /* SOLID_ROD */
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
	if (phys.circle)
		mw("body", 1, circle_def.radius);
	else
		mw("body", 2, box_def.extents.x, box_def.extents.y);
	mw("body", 8, shape_def->density, shape_def->friction, shape_def->restitution, body_def.position.x, body_def.position.y, body_def.rotation, body_def.linearDamping, body_def.angularDamping);
	bd->body = the_world->CreateBody(&body_def);
}

static block_descr *find_block_by_id(int id)
{
	for (int i = 0; i < the_block_cnt; i++) {
		if (the_blocks[i].block->id == id)
			return &the_blocks[i];
	}
	return NULL;
}

static joint_collection *get_closest_jc(double x, double y, int joints[2])
{
	double best_dist = 10000000.0;
	joint_collection *res = NULL;

	for (int i = 0; i < 2; i++) {
		int block_idx = joints[i];
		if (block_idx < 0)
			continue;
		block_descr *bd = find_block_by_id(block_idx);
		if (!bd)
			continue;
		for (joint_descr *j = bd->joints; j; j = j->next) {
			double dist = distance(x, y, j->jc->x, j->jc->y);
			if (dist < best_dist) {
				best_dist = dist;
				res = j->jc;
			}
		}
	}

	return res;
}

void get_rod_endpoints(fcsim_block *block, double *x0, double *y0, double *x1, double *y1)
{
	mw("rod_ends_in", 4, block->x, block->y, block->w, block->angle);

	double cw = cos(block->angle) * block->w / 2;
	double sw = sin(block->angle) * block->w / 2;

	*x0 = block->x - cw;
	*y0 = block->y - sw;
	*x1 = block->x + cw;
	*y1 = block->y + sw;
	
	mw("rod_ends_out", 4, *x0, *y0, *x1, *y1);
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

static void create_joint(b2Body *b1, b2Body *b2, double x, double y, int type)
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
	mw("joint", 4, joint_def.anchorPoint.x, joint_def.anchorPoint.y, joint_def.motorTorque, joint_def.motorSpeed);
	the_world->CreateJoint(&joint_def);
}

static void create_block_joint(block_descr *bd, joint_descr *j)
{
	joint_collection *jc = j->jc;
	block_descr *to = j->to_block;
	if (!to)
		return;

	int type = joint_type(bd->block->type);
	if (jc->cnt == 2 && is_wheel(to->block->type) && jc->x == to->block->x && jc->y == to->block->y)
		type = joint_type(to->block->type);
	if (type != FCSIM_JOINT_PIN && is_wheel(bd->block->type))
		type = -type;

	create_joint(to->body, bd->body, jc->x, jc->y, type);
}

static void rec(joint_descr *j, block_descr *bd)
{
	if (!j)
		return;
	rec(j->next, bd);
	create_block_joint(bd, j);
}

static void create_block_joints(block_descr *bd)
{
	/*
	for (joint_descr *j = bd->joints; j; j = j->next)
		create_block_joint(bd, j);
	*/
	rec(bd->joints, bd);
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

static void make_block_joint(block_descr *bd, double x, double y)
{
	joint_collection *jc = &the_joints[the_joint_cnt++];

	jc->x = x;
	jc->y = y;
	jc->cnt = 0;
	jc->top_block = NULL;

	connect_joint(bd, jc);
}

static void make_connecting_block_joint(block_descr *bd, joint_collection *jc, double *x, double *y)
{
	if (jc) {
		*x = jc->x;
		*y = jc->y;
		connect_joint(bd, jc);
	} else {
		make_block_joint(bd, *x, *y);
	}
}

int cnt = 0;
static void do_rod_joints(struct block_descr *bd)
{
	fcsim_block *block = bd->block;
	double x0, y0, x1, y1;

	get_rod_endpoints(block, &x0, &y0, &x1, &y1);
	joint_collection *j0 = get_closest_jc(x0, y0, block->joints);
	joint_collection *j1 = get_closest_jc(x1, y1, block->joints);

	if (j0) mw("jj", 2, j0->x, j0->y);
	if (j1) mw("jj", 2, j1->x, j1->y);
	mw("jj", 0);

	if (j0 && j1 && share_block(j0, j1))
		j1 = NULL;

	if (j0) mw("jj", 2, j0->x, j0->y);
	if (j1) mw("jj", 2, j1->x, j1->y);
	mw("jj", 0);

	if (j0 && !j1) {
		make_connecting_block_joint(bd, j1, &x1, &y1);
		make_connecting_block_joint(bd, j0, &x0, &y0);
	} else {
		make_connecting_block_joint(bd, j0, &x0, &y0);
		make_connecting_block_joint(bd, j1, &x1, &y1);
	}

	mw("atan2_in", 2, y1 - y0, x1 - x0);
	block->angle = atan2(y1 - y0, x1 - x0);
	mw("atan2_out", 1, block->angle);
	block->w = distance(x0, y0, x1, y1);
	block->x = x0 + (x1 - x0) / 2.0;
	block->y = y0 + (y1 - y0) / 2.0;
}
		
static void do_wheel_joints(struct block_descr *bd)
{
	fcsim_block *block = bd->block;
	double x = block->x;
	double y = block->y;
	double r = block->w/2;
	joint_collection *j = get_closest_jc(x, y, block->joints);

	// todo: bogus joint order
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
	double x = block->x;
	double y = block->y;
	double w = block->w/2;
	double h = block->h/2;

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

	if (block->id == -1)
		mw("level_block", 5, block->x, block->y, block->w, block->h, block->angle);
	else
		mw("player_block", 5, block->x, block->y, block->w, block->h, block->angle);

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
		double angle = bd->body->GetRotation();

		bd->block->x = pos.x;
		bd->block->y = pos.y;
		bd->block->angle = angle;
	}

	for (int i = the_block_cnt - 1; i >= 0; i--) {
		//mw("step_block", 3, the_blocks[i].block->x, the_blocks[i].block->y, the_blocks[i].block->angle);
	}

}
