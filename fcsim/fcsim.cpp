#include <stdio.h>
#include <fcsim.h>
#include <fcsim_funcs.h>
#include "box2d/Include/Box2D.h"

struct block;

#define JOINT_PIN  0
#define JOINT_CW   1
#define JOINT_CCW -1

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

struct fcsim_handle {
	fcsim_arena *arena;
	b2World *world;
	block *blocks;
	int block_cnt;
};



class collision_filter : public b2CollisionFilter {
	bool ShouldCollide(b2Shape *s1, b2Shape *s2)
	{
		if (!b2_defaultFilter.ShouldCollide(s1, s2))
			return false;

		block *b1 = (block *)s1->GetUserData();
		block *b2 = (block *)s2->GetUserData();

		joint_collection_list *j1, *j2;
		for (j1 = b1->jcs_head; j1; j1 = j1->next) {
			for (j2 = b2->jcs_head; j2; j2 = j2->next) {
				if (j1->jc == j2->jc)
					return false;
			}
		}

		return true;
	}
};

static collision_filter fcsim_collision_filter;



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

static void generate_body(b2World *world, block *b)
{
	fcsim_block_def *bdef = b->bdef;
	block_physics phys = block_physics_tbl[bdef->type];
	b2BoxDef box_def;
	b2CircleDef circle_def;
	b2ShapeDef *shape_def;
	b2BodyDef body_def;
	double rotation = bdef->angle;

	if (phys.circle) {
		if (is_wheel(b)) {
			circle_def.radius = bdef->w/2;
		} else {
			rotation = 0.0;
			circle_def.radius = bdef->w;
		}
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
	body_def.rotation = rotation;
	body_def.linearDamping = phys.linearDamping;
	body_def.angularDamping = phys.angularDamping;
	body_def.AddShape(shape_def);
	b->body = world->CreateBody(&body_def);
}

static void generate_joint(b2World *world, joint *j)
{
	if (j->generated)
		return;
	j->generated = true;

	b2RevoluteJointDef joint_def;
	joint_def.body1 = j->block1->body;
	joint_def.body2 = j->block2->body;
	joint_def.anchorPoint.Set(j->x, j->y);
	joint_def.collideConnected = true;
	if (j->type != JOINT_PIN) {
		joint_def.motorTorque = 50000000;
		joint_def.motorSpeed = -5 * j->type;
		joint_def.enableMotor = true;
	}
	world->CreateJoint(&joint_def);
}



static bool is_player(fcsim_block_def *bdef)
{
	switch (bdef->type) {
	case FCSIM_STAT_RECT:
	case FCSIM_STAT_CIRCLE:
	case FCSIM_DYN_RECT:
	case FCSIM_DYN_CIRCLE:
		return false;
	}
	return true;
}



static block *find_block_by_id(fcsim_handle *handle, int id)
{
	for (int i = 0; i < handle->block_cnt; i++) {
		if (handle->blocks[i].bdef->id == id)
			return &handle->blocks[i];
	}
	return NULL;
}

static double distance(double x1, double y1, double x2, double y2)
{
	double dx = x2 - x1;
	double dy = y2 - y1;

	return sqrt(dx * dx + dy * dy);
}

static joint_collection *get_closest_jc(fcsim_handle *handle, double x, double y, fcsim_block_def *bdef)
{
	//double best_dist = 10000000.0;
	double best_dist = 10.0;
	joint_collection *res = NULL;

	for (int i = 0; i < bdef->joint_cnt; i++) {
		block *b = find_block_by_id(handle, bdef->joints[i]);
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
	double cw = fcsim_cos(bdef->angle) * bdef->w / 2;
	double sw = fcsim_sin(bdef->angle) * bdef->w / 2;

	*x0 = bdef->x - cw;
	*y0 = bdef->y - sw;
	*x1 = bdef->x + cw;
	*y1 = bdef->y + sw;
}

int share_block(fcsim_handle *handle, joint_collection *jc0, joint_collection *jc1)
{
	for (int i = 0; i < handle->block_cnt; i++) {
		bool f0 = false, f1 = false;
		for (joint_collection_list *j = handle->blocks[i].jcs_head; j; j = j->next) {
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
		case FCSIM_CW_WHEEL:  return JOINT_CW;
		case FCSIM_CCW_WHEEL: return JOINT_CCW;
	}
	return JOINT_PIN;
}

static joint_collection_list *create_joint(block *b, double x, double y)
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

static void create_rod_joints(block *b, fcsim_handle *handle)
{
	fcsim_block_def *bdef = b->bdef;
	double x0, y0, x1, y1;

	get_rod_endpoints(bdef, &x0, &y0, &x1, &y1);
	joint_collection *jc0 = get_closest_jc(handle, x0, y0, bdef);
	joint_collection *jc1 = get_closest_jc(handle, x1, y1, bdef);

	if (jc0 && jc1 && share_block(handle, jc0, jc1))
		jc1 = NULL;

	if (jc0) {
		x0 = jc0->x;
		y0 = jc0->y;
	}

	if (jc1) {
		x1 = jc1->x;
		y1 = jc1->y;
	}

	bdef->angle = fcsim_atan2(y1 - y0, x1 - x0);
	bdef->w = distance(x0, y0, x1, y1);
	bdef->x = x0 + (x1 - x0) / 2.0;
	bdef->y = y0 + (y1 - y0) / 2.0;
	
	joint_collection_list *jcl0 = create_joint(b, x0, y0);
	joint_collection_list *jcl1 = create_joint(b, x1, y1);

	if (jc0)
		replace_joint(b, jcl0, jc0);

	if (jc1)
		replace_joint(b, jcl1, jc1);
}



static void create_wheel_joints(block *b, fcsim_handle *handle)
{
	fcsim_block_def *bdef = b->bdef;
	double x = bdef->x;
	double y = bdef->y;
	double r = bdef->w/2;

	joint_collection *jc = get_closest_jc(handle, x, y, bdef);
	if (jc) {
		x = bdef->x = jc->x;
		y = bdef->y = jc->y;
	}

	double a[4] = {
		0.0,
		3.141592653589793 / 2,
		3.141592653589793,
		4.71238898038469,
	};

	joint_collection_list *jcl = create_joint(b, bdef->x, bdef->y);
	for (int i = 0; i < 4; i++) {
		create_joint(b, x + fcsim_cos(bdef->angle + a[i]) * r,
		                y + fcsim_sin(bdef->angle + a[i]) * r);
	}

	if (jc)
		replace_joint(b, jcl, jc);
}

static void create_goal_rect_joints(block *b)
{
	fcsim_block_def *bdef = b->bdef;
	double x = bdef->x;
	double y = bdef->y;
	double w_half = bdef->w/2;
	double h_half = bdef->h/2;

	double x0 =  fcsim_cos(bdef->angle) * w_half;
	double y0 =  fcsim_sin(bdef->angle) * w_half;
	double x1 =  fcsim_sin(bdef->angle) * h_half;
	double y1 = -fcsim_cos(bdef->angle) * h_half;

	create_joint(b, x, y);
	create_joint(b, x + x0 + x1, y + y0 + y1);
	create_joint(b, x - x0 + x1, y - y0 + y1);
	create_joint(b, x + x0 - x1, y + y0 - y1);
	create_joint(b, x - x0 - x1, y - y0 - y1);
}

static void create_joints(block *b, fcsim_handle *handle)
{
	switch (b->bdef->type) {
	case FCSIM_GOAL_RECT:
		create_goal_rect_joints(b);
		return;
	case FCSIM_GOAL_CIRCLE:
	case FCSIM_WHEEL:
	case FCSIM_CW_WHEEL:
	case FCSIM_CCW_WHEEL:
		create_wheel_joints(b, handle);
		return;
	case FCSIM_ROD:
	case FCSIM_SOLID_ROD:
		create_rod_joints(b, handle);
		return;
	}
}

void add_block(fcsim_handle *handle, fcsim_block_def *bdef)
{
	block *block = &handle->blocks[handle->block_cnt++];
	memset(block, 0, sizeof(*block));
	block->bdef = bdef;

	create_joints(block, handle);
}

fcsim_arena *copy_arena(fcsim_arena *arena)
{
	fcsim_arena *new_arena;

	new_arena = new fcsim_arena;
	new_arena->blocks = new fcsim_block_def[arena->block_cnt];
	memcpy(new_arena->blocks, arena->blocks, sizeof(fcsim_block_def) * arena->block_cnt);
	new_arena->block_cnt = arena->block_cnt;
	new_arena->build = arena->build;
	new_arena->goal = arena->goal;

	return new_arena;
}

fcsim_handle *fcsim_new(fcsim_arena *arena)
{
	fcsim_handle *handle = new fcsim_handle;
	fcsim_arena *new_arena;

	new_arena = copy_arena(arena);

	b2Vec2 gravity(0, 300);
	b2AABB aabb;
	aabb.minVertex.Set(-2000, -1450);
	aabb.maxVertex.Set(2000, 1450);
	handle->world = new b2World(aabb, gravity, true);
	handle->world->SetFilter(&fcsim_collision_filter);

	handle->blocks = new block[new_arena->block_cnt];
	handle->block_cnt = 0;

	handle->arena = new_arena;

	for (int i = 0; i < new_arena->block_cnt; i++) {
		if (is_player(&new_arena->blocks[i]))
			add_block(handle, &new_arena->blocks[i]);
	}

	for (int i = 0; i < new_arena->block_cnt; i++) {
		if (!is_player(&new_arena->blocks[i]))
			add_block(handle, &new_arena->blocks[i]);
	}

	for (int i = 0; i < handle->block_cnt; i++)
		generate_body(handle->world, &handle->blocks[i]);

	for (int i = 0; i < handle->block_cnt; i++) {
		block *b = &handle->blocks[i];
		for (joint_collection_list *l = b->jcs_head; l; l = l->next) {
			for (joint *j = l->jc->joints_head; j; j = j->next)
				generate_joint(handle->world, j);
		}
	}

	return handle;
}

void fcsim_step(fcsim_handle *handle)
{
	handle->world->Step(1.0/30.0, 10);

	b2Joint *joint = handle->world->GetJointList();
	while (joint) {
		b2Joint *next = joint->GetNext();
		b2Vec2 a1 = joint->GetAnchor1();
		b2Vec2 a2 = joint->GetAnchor2();
		b2Vec2 d = a1 - a2;
		if (fabs(d.x) + fabs(d.y) > 50.0)
			handle->world->DestroyJoint(joint);
		joint = next;
	}
}

void fcsim_get_block_stats(struct fcsim_handle *handle,
			   struct fcsim_block_stat *stats)
{
	for (int i = 0; i < handle->block_cnt; i++) {
		block *b = &handle->blocks[i];
		b2Vec2 pos   = b->body->GetOriginPosition();
		double angle = b->body->GetRotation();

		int ind = b->bdef - handle->arena->blocks;

		stats[ind].x = pos.x;
		stats[ind].y = pos.y;
		stats[ind].angle = angle;
	}
}

static void rect_block_bb(fcsim_block_def *block, fcsim_rect *rect)
{
	float sina = fcsim_sin(block->angle);
	float cosa = fcsim_cos(block->angle);
	float wc = block->w * cosa;
	float ws = block->w * sina;
	float hc = block->h * cosa;
	float hs = block->h * sina;

	rect->x = block->x;
	rect->y = block->y;
	rect->w = fabs(wc) + fabs(hs);
	rect->h = fabs(ws) + fabs(hc);
}

static void circle_block_bb(fcsim_block_def *block, fcsim_rect *rect)
{
	rect->x = block->x;
	rect->y = block->y;
	rect->w = block->w;
	rect->h = block->w;
}

static bool is_circle(fcsim_block_def *bdef)
{
	switch (bdef->type) {
	case FCSIM_STAT_RECT:
	case FCSIM_DYN_RECT:
	case FCSIM_GOAL_RECT:
	case FCSIM_ROD:
	case FCSIM_SOLID_ROD:
		return false;
	}
	return true;
}

static void block_bb(fcsim_block_def *block, fcsim_rect *rect)
{
	if (is_circle(block))
		circle_block_bb(block, rect);
	else
		rect_block_bb(block, rect);
}

static int block_inside_rect(fcsim_block_def *block, fcsim_rect *rect)
{
	fcsim_rect bb;

	block_bb(block, &bb);

	return bb.x - bb.w / 2 >= rect->x - rect->w / 2
	    && bb.x + bb.w / 2 <= rect->x + rect->w / 2
	    && bb.y - bb.h / 2 >= rect->y - rect->h / 2
	    && bb.y + bb.h / 2 <= rect->y + rect->h / 2;
}

int fcsim_has_won(fcsim_arena *arena, struct fcsim_block_stat *stats)
{
	struct fcsim_block_def block;
	bool checked = false;

	for (int i = 0; i < arena->block_cnt; i++) {
		block = arena->blocks[i];
		block.x = stats[i].x;
		block.y = stats[i].y;
		block.angle = stats[i].angle;
		if (block.type == FCSIM_GOAL_RECT ||
		    block.type == FCSIM_GOAL_CIRCLE) {
			if (!block_inside_rect(&block, &arena->goal))
				return false;
			checked = true;
		}
	}

	return checked;
}

struct fcsim_arena *fcsim_arena_from_handle(struct fcsim_handle *handle)
{
	return handle->arena;
}