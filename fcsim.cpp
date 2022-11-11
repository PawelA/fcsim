#include <vector>
#include <stdio.h>

#include "box2d/Include/Box2D.h"

#include "fcsim.h"

#define ARENA_WIDTH	2000
#define ARENA_HEIGHT	1450
#define TIME_STEP	0.03333333333333333
#define ITERATIONS	10

struct joint_collection {
	float x, y;
	int top_block_idx;
	int cnt;
};

struct block_info {
	struct fcsim_block *block;
	b2Body *body;
	std::vector<int> joint_idxs;
};

struct fcsim_world {
	b2World world;
	std::vector<struct joint_collection> joints;
	std::vector<struct block_info> blocks;

	fcsim_world(b2AABB aabb, b2Vec2 gravity) :
		world(aabb, gravity, true) {}
};

static float distance(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;

	return sqrtf(dx * dx + dy * dy);
}

struct fcsim_world *g_world;

class fcsim_collision_filter : public b2CollisionFilter
{
public:
	bool ShouldCollide(b2Shape *shape1, b2Shape *shape2)
	{
		if (!b2_defaultFilter.ShouldCollide(shape1, shape2))
			return false;
		int b_idx1 = reinterpret_cast<long>(shape1->GetUserData());
		int b_idx2 = reinterpret_cast<long>(shape2->GetUserData());

		if (b_idx1 >= g_world->blocks.size())
			return false;
		if (b_idx2 >= g_world->blocks.size())
			return false;

		printf(">> %d %d\n", b_idx1, b_idx2);

		for (int j1 : g_world->blocks[b_idx1].joint_idxs) {
			printf("[%d]\n", j1);
			for (int j2 : g_world->blocks[b_idx2].joint_idxs) {
				printf("[%d %d]\n", j1, j2);
				if (j1 == j2)
					return false;
			}
		}

		return true;
	}
};

static fcsim_collision_filter collision_filter;

fcsim_world *fcsim_create_world(void)
{
	struct fcsim_world *world;
	b2AABB aabb;
	b2Vec2 gravity(0, 300);

	aabb.minVertex.Set(-ARENA_WIDTH, -ARENA_HEIGHT);
	aabb.maxVertex.Set( ARENA_WIDTH,  ARENA_HEIGHT);

	world = new fcsim_world(aabb, gravity);
	world->world.SetFilter(&collision_filter);

	g_world = world;

	return world;
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

static b2Body *create_block_body(b2World *world, struct block_info *bi)
{
	struct fcsim_block *block = bi->block;
	struct block_physics phys = block_physics_tbl[block->type];
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
	shape_def->userData = reinterpret_cast<void *>(g_world->blocks.size());
	body_def.position.Set(block->x, block->y);
	body_def.rotation = block->angle;
	body_def.linearDamping = phys.linearDamping;
	body_def.angularDamping = phys.angularDamping;
	body_def.AddShape(shape_def);
	return world->CreateBody(&body_def);
}

static int get_closest_joint_collection(struct fcsim_world *world, float x,
					float y, int block_idxs[2])
{
	float best_dist = 10.0f;
	int res = -1;

	for (int i = 0; i < 2; i++) {
		int block_idx = block_idxs[i];
		if (block_idx < 0)
			continue;
		printf(".\n");
		for (int joint_idx : world->blocks[block_idx].joint_idxs) {
			struct joint_collection *joint = &world->joints[joint_idx];
			float jx = joint->x, jy = joint->y;
			printf("%f %f %f %f\n", x, y, jx, jy);
			float dist = distance(x, y, jx, jy);
			if (dist < best_dist) {
				best_dist = dist;
				res = joint_idx;
			}
		}
	}

	return res;
}

void get_rod_endpoints(struct fcsim_block *block, float *x0, float *y0, float *x1, float *y1)
{
	float cw = cosf(block->angle) * block->w / 2;
	float sw = sinf(block->angle) * block->w / 2;

	*x0 = block->x - cw;
	*y0 = block->y - sw;
	*x1 = block->x + cw;
	*y1 = block->y + sw;
}

int share_block(struct fcsim_world *world, int j0_idx, int j1_idx)
{
	for (struct block_info bi : world->blocks) {
		bool f0 = false, f1 = false;
		for (int j_idx : bi.joint_idxs) {
			if (j0_idx == j_idx)
				f0 = true;
			if (j1_idx == j_idx)
				f1 = true;
		}
		if (f0 && f1)
			return true;
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

static void create_joint(b2World *world, b2Body *b1, b2Body *b2, float x, float y, int type)
{
	b2RevoluteJointDef joint_def;

	printf("Peekaboo!\n");

	joint_def.body1 = b1;
	joint_def.body2 = b2;
	joint_def.anchorPoint.Set(x, y);
	joint_def.collideConnected = true;
	if (type != FCSIM_JOINT_PIN) {
		joint_def.motorTorque = 50000000;
		joint_def.motorSpeed = -5 * type;
		joint_def.enableMotor = true;
	}
	world->CreateJoint(&joint_def);
}

static void connect_joint(struct fcsim_world *world, struct block_info *bi, int j_idx)
{
	struct joint_collection *j = &world->joints[j_idx];
	struct block_info *bi2 = &world->blocks[j->top_block_idx];

	int j_type = joint_type(bi->block->type);
	if (j->cnt == 1 && is_wheel(bi2->block->type))
		j_type = joint_type(bi2->block->type);
	if (j_type != FCSIM_JOINT_PIN && is_wheel(bi->block->type))
		j_type = -j_type;

	create_joint(&world->world, bi2->body, bi->body, j->x, j->y, j_type);

	j->top_block_idx = world->blocks.size();
	j->cnt++;

	bi->joint_idxs.push_back(j_idx);
}

static void new_joint_collection(struct fcsim_world *world, float x, float y, struct block_info *bi)
{
	struct joint_collection j;
	printf("new_joint_collection(%f, %f)\n", x, y);
	j.x = x;
	j.y = y;
	j.top_block_idx = world->blocks.size();
	j.cnt = 1;
	bi->joint_idxs.push_back(world->joints.size());
	world->joints.push_back(j);
}

static void do_rod_joints(struct fcsim_world *world, struct block_info *bi)
{
	struct fcsim_block *block = bi->block;
	float x0, y0, x1, y1;

	get_rod_endpoints(block, &x0, &y0, &x1, &y1);
	int j0_idx = get_closest_joint_collection(world, x0, y0, block->joints);
	int j1_idx = get_closest_joint_collection(world, x1, y1, block->joints);

	if (j0_idx == j1_idx)
		j1_idx = -1;
	if (j0_idx >= 0 && j1_idx >= 0 && share_block(world, j0_idx, j1_idx))
		j1_idx = -1;

	if (j0_idx >= 0)
		connect_joint(world, bi, j0_idx);
	else
		new_joint_collection(world, x0, y0, bi);
	
	if (j1_idx >= 0)
		connect_joint(world, bi, j1_idx);
	else
		new_joint_collection(world, x1, y1, bi);
}
		
static void do_wheel_joints(struct fcsim_world *world, struct block_info *bi)
{
	struct fcsim_block *block = bi->block;
	float x = block->x;
	float y = block->y;
	int j_idx = get_closest_joint_collection(world, x, y, block->joints);

	if (j_idx >= 0)
		connect_joint(world, bi, j_idx);
	else
		new_joint_collection(world, x, y, bi);
	new_joint_collection(world, x + block->w/2, y, bi);
	new_joint_collection(world, x - block->w/2, y, bi);
	new_joint_collection(world, x, y + block->w/2, bi);
	new_joint_collection(world, x, y - block->w/2, bi);
}

static void do_goal_rect_joints(struct fcsim_world *world, struct block_info *bi)
{
	struct fcsim_block *block = bi->block;
	float x = block->x;
	float y = block->y;
	float w = block->w/2;
	float h = block->h/2;

	new_joint_collection(world, x, y, bi);
	new_joint_collection(world, x + w, y + w, bi);
	new_joint_collection(world, x + w, y - w, bi);
	new_joint_collection(world, x - w, y + w, bi);
	new_joint_collection(world, x - w, y - w, bi);
}

static void do_joints(struct fcsim_world *world, struct block_info *block)
{
	switch (block->block->type) {
	case FCSIM_GOAL_RECT:
		do_goal_rect_joints(world, block);
	case FCSIM_GOAL_CIRCLE:
	case FCSIM_WHEEL:
	case FCSIM_CW_WHEEL:
	case FCSIM_CCW_WHEEL:
		do_wheel_joints(world, block);
		return;
	case FCSIM_ROD:
	case FCSIM_SOLID_ROD:
		do_rod_joints(world, block);
		return;
	}
}

int fcsim_add_block(struct fcsim_world *world, struct fcsim_block *block)
{
	struct joint_collection joint;
	struct block_info block_info;

	assert(block->type >= 0 && block->type <= FCSIM_TYPE_MAX);
	assert(block->joints[0] < (int)world->joints.size());
	assert(block->joints[1] < (int)world->joints.size());

	if (block->type == FCSIM_STAT_CIRCLE || block->type == FCSIM_DYN_CIRCLE) {
		block->w *= 2;
		block->h *= 2;
	}

	block_info.block = block;
	block_info.body = create_block_body(&world->world, &block_info);
	do_joints(world, &block_info);
	world->blocks.push_back(block_info);

	return 1;
}

void fcsim_step(struct fcsim_world *world)
{
	world->world.Step(TIME_STEP, ITERATIONS);

	for (auto block_info : world->blocks) {
		b2Vec2 pos = block_info.body->GetOriginPosition();
		float angle = block_info.body->GetRotation();

		block_info.block->x = pos.x;
		block_info.block->y = pos.y;
		block_info.block->angle = angle;
	}
}
