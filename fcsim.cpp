#include "box2d/Include/Box2D.h"
#include "fcsim.h"
#include "net.h"

#define ARENA_WIDTH	2000
#define ARENA_HEIGHT	1450
#define TIME_STEP	0.03333333333333333
#define ITERATIONS	10

#define JOINT_PIN  0
#define JOINT_CW   1
#define JOINT_CCW -1

struct block;

struct joint {
	double x, y;
	int type;
	bool generated;
	block *block1;
	block *block2;
	joint *next;
};

struct joint_collection {
	double x, y;
	block *top_block;
	joint *joints_head;
	joint *joints_tail;
};

struct joint_collection_list {
	joint_collection *jc;
	joint_collection_list *next;
};

struct block {
	fcsim_block_def *bdef;
	b2Body *body;
	joint_collection_list *jcs_head;
	joint_collection_list *jcs_tail;
};

b2World *the_world;

block the_blocks[1000];
int   the_block_cnt;

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

		block *b1 = (block *)s1->GetUserData();
		block *b2 = (block *)s2->GetUserData();

		for (joint_collection_list *j1 = b1->jcs_head; j1; j1 = j1->next) {
			for (joint_collection_list *j2 = b2->jcs_head; j2; j2 = j2->next) {
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
	/* c dens fric rest   cB   mB   linD angD */
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

static void generate_body(block *b)
{
	fcsim_block_def *bdef = b->bdef;
	block_physics phys = block_physics_tbl[bdef->type];
	b2BoxDef box_def;
	b2CircleDef circle_def;
	b2ShapeDef *shape_def;
	b2BodyDef body_def;

	if (phys.circle) {
		circle_def.radius = bdef->w/2;
		shape_def = &circle_def;
	} else {
		box_def.extents.Set(bdef->w/2, bdef->h/2);
		shape_def = &box_def;
	}
	shape_def->density = phys.density;
	shape_def->friction = phys.friction;
	shape_def->restitution = phys.restitution;
	shape_def->categoryBits = phys.categoryBits;
	shape_def->maskBits = phys.maskBits;
	shape_def->userData = b;
	body_def.position.Set(bdef->x, bdef->y);
	body_def.rotation = bdef->angle;
	body_def.linearDamping = phys.linearDamping;
	body_def.angularDamping = phys.angularDamping;
	body_def.AddShape(shape_def);
	if (phys.circle)
		mw("body", 1, circle_def.radius);
	else
		mw("body", 2, box_def.extents.x, box_def.extents.y);
	mw("body", 8, shape_def->density, shape_def->friction, shape_def->restitution, body_def.position.x, body_def.position.y, body_def.rotation, body_def.linearDamping, body_def.angularDamping);
	b->body = the_world->CreateBody(&body_def);
}

static block *find_block_by_id(int id)
{
	for (int i = 0; i < the_block_cnt; i++) {
		if (the_blocks[i].bdef->id == id)
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
		block *b = find_block_by_id(block_idx);
		if (!b)
			continue;
		for (joint_collection_list *j = b->jcs_head; j; j = j->next) {
			double dist = distance(x, y, j->jc->x, j->jc->y);
			if (dist < best_dist) {
				best_dist = dist;
				res = j->jc;
			}
		}
	}

	return res;
}

void get_rod_endpoints(fcsim_block_def *bdef, double *x0, double *y0, double *x1, double *y1)
{
	mw("rod_ends_in", 4, bdef->x, bdef->y, bdef->w, bdef->angle);

	double cw = cos(bdef->angle) * bdef->w / 2;
	double sw = sin(bdef->angle) * bdef->w / 2;

	*x0 = bdef->x - cw;
	*y0 = bdef->y - sw;
	*x1 = bdef->x + cw;
	*y1 = bdef->y + sw;
	
	mw("rod_ends_out", 4, *x0, *y0, *x1, *y1);
}

int share_block(joint_collection *jc0, joint_collection *jc1)
{
	for (int i = 0; i < the_block_cnt; i++) {
		bool f0 = false, f1 = false;
		for (joint_collection_list *j = the_blocks[i].jcs_head; j; j = j->next) {
			if (j->jc == jc0) f0 = true;
			if (j->jc == jc1) f1 = true;
		}
		if (f0 && f1) return true;
	}
	return false;
}

static int joint_type(int block_type)
{
	switch (block_type) {
		case FCSIM_CW_WHEEL: return  JOINT_CW;
		case FCSIM_CCW_WHEEL: return JOINT_CCW;
	}
	return JOINT_PIN;
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

static void generate_joint(joint *j)
{
	b2RevoluteJointDef joint_def;

	if (j->generated)
		return;
	j->generated = true;

	joint_def.body1 = j->block1->body;
	joint_def.body2 = j->block2->body;
	joint_def.anchorPoint.Set(j->x, j->y);
	joint_def.collideConnected = true;
	if (j->type != JOINT_PIN) {
		joint_def.motorTorque = 50000000;
		joint_def.motorSpeed = -5 * j->type;
		joint_def.enableMotor = true;
	}
	the_world->CreateJoint(&joint_def);

	mw("joint", 4, joint_def.anchorPoint.x, joint_def.anchorPoint.y, joint_def.motorTorque, joint_def.motorSpeed);
}

static joint_collection_list *add_joint(block *b, double x, double y)
{
	joint_collection *jc = new joint_collection;
	jc->x = x;
	jc->y = y;
	jc->top_block = b;
	jc->joints_head = NULL;
	jc->joints_tail = NULL;

	joint_collection_list *jcl = new joint_collection_list;
	jcl->jc = jc;
	jcl->next = NULL;

	if (b->jcs_tail) {
		b->jcs_tail->next = jcl;
		b->jcs_tail = jcl;
	} else {
		b->jcs_head = jcl;
		b->jcs_tail = jcl;
	}

	return jcl;
}

static bool is_wheel(block *b)
{
	switch (b->bdef->type) {
	case FCSIM_GOAL_CIRCLE:
	case FCSIM_WHEEL:
	case FCSIM_CW_WHEEL:
	case FCSIM_CCW_WHEEL:
		return true;
	}
	return false;
}

static int joint_type(block *b)
{
	switch (b->bdef->type) {
		case FCSIM_CW_WHEEL:  return JOINT_CW;
		case FCSIM_CCW_WHEEL: return JOINT_CCW;
	}
	return JOINT_PIN;
}

static bool is_singular_wheel_center(joint_collection *jc)
{
	if (jc->joints_head)
		return false;

	if (!is_wheel(jc->top_block))
		return false;

	return jc->x == jc->top_block->bdef->x &&
	       jc->y == jc->top_block->bdef->y;
}

static int get_joint_type(block *b, joint_collection *jc)
{
	block *top = jc->top_block;
	int type;

	if (is_singular_wheel_center(jc))
		type = joint_type(jc->top_block);
	else
		type = joint_type(b);

	if (is_wheel(b))
		type = -type;

	return type;
}

static joint_collection_list *jcl_swap(joint_collection_list *jcl)
{
	joint_collection_list *new_jcl = jcl->next;

	if (!new_jcl)
		return jcl;

	jcl->next = new_jcl->next;
	new_jcl->next = jcl;

	return new_jcl;
}

static void replace_joint(block *b, joint_collection_list *jcl, joint_collection *jc)
{
	delete jcl->jc;
	jcl->jc = jc;

	if (b->jcs_head == jcl) {
		b->jcs_head = jcl_swap(b->jcs_head);
	} else {
		for (joint_collection_list *l = b->jcs_head; l; l = l->next) {
			if (l->next == jcl) {
				l->next = jcl_swap(l->next);
				break;
			}
		}
	}

	joint *j = new joint;
	j->x = jc->x;
	j->y = jc->y;
	j->type = get_joint_type(b, jc);
	j->generated = false;
	j->block1 = jc->top_block;
	j->block2 = b;
	j->next = NULL;

	jc->top_block = b;
	if (jc->joints_tail) {
		jc->joints_tail->next = j;
		jc->joints_tail = j;
	} else {
		jc->joints_head = j;
		jc->joints_tail = j;
	}
}

static void do_rod_joints(block *b)
{
	fcsim_block_def *bdef = b->bdef;
	double x0, y0, x1, y1;

	get_rod_endpoints(bdef, &x0, &y0, &x1, &y1);
	joint_collection *jc0 = get_closest_jc(x0, y0, bdef->joints);
	joint_collection *jc1 = get_closest_jc(x1, y1, bdef->joints);

	if (jc0 && jc1 && share_block(jc0, jc1))
		jc1 = NULL;

	if (jc0) {
		x0 = jc0->x;
		y0 = jc0->y;
	}

	if (jc1) {
		x1 = jc1->x;
		y1 = jc1->y;
	}

	bdef->angle = atan2(y1 - y0, x1 - x0);
	bdef->w = distance(x0, y0, x1, y1);
	bdef->x = x0 + (x1 - x0) / 2.0;
	bdef->y = y0 + (y1 - y0) / 2.0;

	joint_collection_list *jcl0 = add_joint(b, x0, y0);
	joint_collection_list *jcl1 = add_joint(b, x1, y1);

	if (jc0)
		replace_joint(b, jcl0, jc0);

	if (jc1)
		replace_joint(b, jcl1, jc1);
}
		
static void do_wheel_joints(block *b)
{
	fcsim_block_def *bdef = b->bdef;
	double x = bdef->x;
	double y = bdef->y;
	double r = bdef->w/2;

	joint_collection *jc = get_closest_jc(x, y, bdef->joints);
	if (jc) {
		x = bdef->x = jc->x;
		y = bdef->y = jc->y;
	}

	joint_collection_list *jcl = add_joint(b, bdef->x, bdef->y);
	add_joint(b, x + r, y);
	add_joint(b, x, y + r);
	add_joint(b, x - r, y);
	add_joint(b, x, y - r);

	if (jc)
		replace_joint(b, jcl, jc);
}

static void do_goal_rect_joints(block *b)
{
	fcsim_block_def *bdef = b->bdef;
	double x = bdef->x;
	double y = bdef->y;
	double w = bdef->w/2;
	double h = bdef->h/2;

	/* todo: take rotation into account */
	add_joint(b, x, y);
	add_joint(b, x + w, y + w);
	add_joint(b, x + w, y - w);
	add_joint(b, x - w, y + w);
	add_joint(b, x - w, y - w);
}

static void do_joints(block *b)
{
	switch (b->bdef->type) {
	case FCSIM_GOAL_RECT:
		do_goal_rect_joints(b);
		return;
	case FCSIM_GOAL_CIRCLE:
	case FCSIM_WHEEL:
	case FCSIM_CW_WHEEL:
	case FCSIM_CCW_WHEEL:
		do_wheel_joints(b);
		return;
	case FCSIM_ROD:
	case FCSIM_SOLID_ROD:
		do_rod_joints(b);
		return;
	}
}

void fcsim_add_block(fcsim_block_def *bdef)
{
	block *b = &the_blocks[the_block_cnt++];
	b->bdef = bdef;

	assert(bdef->type >= 0 && bdef->type <= FCSIM_TYPE_MAX);
	assert(bdef->joints[0] < the_block_cnt);
	assert(bdef->joints[1] < the_block_cnt);

	if (bdef->id == -1)
		mw("level_block", 5, bdef->x, bdef->y, bdef->w, bdef->h, bdef->angle);
	else
		mw("player_block", 5, bdef->x, bdef->y, bdef->w, bdef->h, bdef->angle);

	if (bdef->type == FCSIM_STAT_CIRCLE || bdef->type == FCSIM_DYN_CIRCLE) {
		bdef->w *= 2;
		bdef->h *= 2;
	}

	do_joints(b);
}

void fcsim_generate(void)
{
	for (int i = 0; i < the_block_cnt; i++)
		generate_body(&the_blocks[i]);
	
	for (int i = 0; i < the_block_cnt; i++) {
		block *b = &the_blocks[i];
		for (joint_collection_list *l = b->jcs_head; l; l = l->next) {
			for (joint *j = l->jc->joints_head; j; j = j->next)
				generate_joint(j);
		}
	}
}

void fcsim_step(void)
{
	the_world->Step(TIME_STEP, ITERATIONS);

	for (int i = 0; i < the_block_cnt; i++) {
		block *b = &the_blocks[i];
		b2Vec2 pos   = b->body->GetOriginPosition();
		double angle = b->body->GetRotation();

		b->bdef->x = pos.x;
		b->bdef->y = pos.y;
		b->bdef->angle = angle;
	}

	for (int i = the_block_cnt - 1; i >= 0; i--) {
		//mw("step_block", 3, the_blocks[i].block->x, the_blocks[i].block->y, the_blocks[i].block->angle);
	}

}
