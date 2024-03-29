#include <stdio.h>
#include <fcsim.h>
#include <Box2D.h>

struct fcsim_simul {
	b2World world;
	int level_block_cnt;
	int player_block_cnt;
	b2Body **level_block_bodies;
	b2Body **player_block_bodies;
};

struct body_data {
	int joint_cnt;
	int joint_ids[5];
};

struct body_list {
	int block_id;
	struct body_list *next;
};

struct joint_map_entry {
	struct fcsim_joint joint;
	int joint_id;
	int generated;
	struct body_list *body_head;
	struct body_list *body_tail;
	struct joint_map_entry *next;
};

struct joint_map {
	struct joint_map_entry *head;
	struct joint_map_entry *tail;
	int next_joint_id;
};

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

void body_list_append(struct joint_map_entry *ent, int block_id)
{
	struct body_list *bl;

	bl = (struct body_list *)calloc(1, sizeof(*bl));
	bl->block_id = block_id;

	if (ent->body_tail)
		ent->body_tail->next = bl;
	else
		ent->body_head = bl;

	ent->body_tail = bl;
}

struct joint_map_entry *joint_map_append(struct joint_map *map, struct fcsim_joint *joint, int joint_id)
{
	struct joint_map_entry *ent;

	ent = (struct joint_map_entry *)calloc(1, sizeof(*ent));
	ent->joint = *joint;
	ent->joint_id = joint_id;

	if (map->tail)
		map->tail->next = ent;
	else
		map->head = ent;

	map->tail = ent;

	return ent;
}

void joint_map_add(struct joint_map *map, struct fcsim_joint *joint, int block_id)
{
	struct joint_map_entry *ent;

	for (ent = map->head; ent; ent = ent->next) {
		if (joints_equal(&ent->joint, joint))
			break;
	}
	if (!ent)
		ent = joint_map_append(map, joint, map->next_joint_id++);
	body_list_append(ent, block_id);
}

struct joint_map_entry *joint_map_find(struct joint_map *map, struct fcsim_joint *joint)
{
	struct joint_map_entry *ent;

	for (ent = map->head; ent; ent = ent->next) {
		if (joints_equal(&ent->joint, joint))
			return ent;
	}

	return NULL;
}

void joint_map_init(struct joint_map *map)
{
	map->head = NULL;
	map->tail = NULL;
	map->next_joint_id = 0;
}

class _collision_filter : public b2CollisionFilter {
	bool ShouldCollide(b2Shape *s1, b2Shape *s2)
	{
		struct body_data *data1;
		struct body_data *data2;
		int i, j;

		if (!b2_defaultFilter.ShouldCollide(s1, s2))
			return false;

		data1 = (struct body_data *)s1->GetUserData();
		data2 = (struct body_data *)s2->GetUserData();

		if (!data1 || !data2)
			return true;

		for (i = 0; i < data1->joint_cnt; i++) {
			for (j = 0; j < data2->joint_cnt; j++) {
				if (data1->joint_ids[i] == data2->joint_ids[j])
					return false;
			}
		}

		return true;
	}
};

static _collision_filter fcsim_collision_filter;

static void init_b2world(b2World *world)
{
	b2Vec2 gravity;
	b2AABB aabb;

	b2Vec2_Set(&gravity, 0, 300);
	b2Vec2_Set(&aabb.minVertex, -2000, -1450);
	b2Vec2_Set(&aabb.maxVertex, 2000, 1450);
	new (world) b2World(aabb, gravity, true);
	world->SetFilter(&fcsim_collision_filter);
}

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

static b2Body *generate_body(b2World *world, struct fcsim_shape *shape, struct fcsim_where *where, struct block_physics *phys, void *data)
{
	b2BoxDef box_def;
	b2CircleDef circle_def;
	b2ShapeDef *shape_def;
	b2BodyDef body_def;
	double x, y, angle;

	if (shape->type == FCSIM_SHAPE_CIRC) {
		circle_def.radius = shape->circ.radius;
		shape_def = &circle_def;
	} else {
		b2Vec2_Set(&box_def.extents, shape->rect.w/2, shape->rect.h/2);
		shape_def = &box_def;
	}
	shape_def->density = phys->density;
	shape_def->friction = phys->friction;
	shape_def->restitution = phys->restitution;
	shape_def->categoryBits = phys->category_bits;
	shape_def->maskBits = phys->mask_bits;
	shape_def->userData = data;
	b2Vec2_Set(&body_def.position, where->x, where->y);
	body_def.rotation = where->angle;
	body_def.linearDamping = phys->linear_damping;
	body_def.angularDamping = phys->angular_damping;
	body_def.AddShape(shape_def);

	return world->CreateBody(&body_def);
}

static void generate_joint(b2World *world, b2Body *b1, b2Body *b2, double x, double y, int spin)
{
	b2RevoluteJointDef joint_def;

	joint_def.body1 = b1;
	joint_def.body2 = b2;
	b2Vec2_Set(&joint_def.anchorPoint, x, y);
	joint_def.collideConnected = true;
	if (spin != 0) {
		joint_def.motorTorque = 50000000;
		joint_def.motorSpeed = spin;
		joint_def.enableMotor = true;
	}
	world->CreateJoint(&joint_def);
}

static void get_rect_desc(struct fcsim_rect *rect, struct fcsim_shape *shape, struct fcsim_where *where)
{
	shape->type = FCSIM_SHAPE_RECT;
	shape->rect.w = rect->w;
	shape->rect.h = rect->h;
	where->x = rect->x;
	where->y = rect->y;
	where->angle = rect->angle;
}

static void get_circ_desc(struct fcsim_circ *circ, struct fcsim_shape *shape, struct fcsim_where *where)
{
	shape->type = FCSIM_SHAPE_CIRC;
	shape->circ.radius = circ->radius;
	where->x = circ->x;
	where->y = circ->y;
	where->angle = 0.0;
}

static double distance(double x1, double y1, double x2, double y2)
{
	double dx = x2 - x1;
	double dy = y2 - y1;

	return sqrt(dx * dx + dy * dy);
}

static void get_jrect_desc(struct fcsim_jrect *jrect, struct fcsim_shape *shape, struct fcsim_where *where)
{
	shape->type = FCSIM_SHAPE_RECT;
	shape->rect.w = jrect->w;
	shape->rect.h = jrect->h;
	where->x = jrect->x;
	where->y = jrect->y;
	where->angle = jrect->angle;
}

static void get_wheel_desc(struct fcsim_level *level, struct fcsim_wheel *wheel, struct fcsim_shape *shape, struct fcsim_where *where)
{
	double x, y;

	fcsim_get_joint_pos(level, &wheel->center, &x, &y);

	shape->type = FCSIM_SHAPE_CIRC;
	shape->circ.radius = wheel->radius;
	where->x = x;
	where->y = y;
	where->angle = wheel->angle;
}

static void get_rod_desc(struct fcsim_level *level, struct fcsim_rod *rod, int solid, struct fcsim_shape *shape, struct fcsim_where *where)
{
	double x0, y0, x1, y1;

	fcsim_get_joint_pos(level, &rod->from, &x0, &y0);
	fcsim_get_joint_pos(level, &rod->to, &x1, &y1);

	shape->type = FCSIM_SHAPE_RECT;
	shape->rect.w = distance(x0, y0, x1, y1);
	shape->rect.h = solid ? 8.0 : 4.0;
	where->x = x0 + (x1 - x0) / 2.0;
	where->y = y0 + (y1 - y0) / 2.0;
	where->angle = fcsim_atan2(y1 - y0, x1 - x0);
}

void fcsim_get_level_block_desc(struct fcsim_level *level, int id, struct fcsim_shape *shape, struct fcsim_where *where)
{
	struct fcsim_block *block = &level->level_blocks[id];

	switch (block->type) {
	case FCSIM_BLOCK_STAT_RECT:
	case FCSIM_BLOCK_DYN_RECT:
		get_rect_desc(&block->rect, shape, where);
		break;
	case FCSIM_BLOCK_STAT_CIRC:
	case FCSIM_BLOCK_DYN_CIRC:
		get_circ_desc(&block->circ, shape, where);
		break;
	}
}

void fcsim_get_player_block_desc(struct fcsim_level *level, int id, struct fcsim_shape *shape, struct fcsim_where *where)
{
	struct fcsim_block *block = &level->player_blocks[id];

	switch (block->type) {
	case FCSIM_BLOCK_GOAL_RECT:
		get_jrect_desc(&block->jrect, shape, where);
		break;
	case FCSIM_BLOCK_GOAL_CIRC:
	case FCSIM_BLOCK_WHEEL:
	case FCSIM_BLOCK_CW_WHEEL:
	case FCSIM_BLOCK_CCW_WHEEL:
		get_wheel_desc(level, &block->wheel, shape, where);
		break;
	case FCSIM_BLOCK_ROD:
		get_rod_desc(level, &block->rod, 0, shape, where);
		break;
	case FCSIM_BLOCK_SOLID_ROD:
		get_rod_desc(level, &block->rod, 1, shape, where);
		break;
	}
}

static void get_block_phys(struct fcsim_block *block, struct block_physics *phys)
{
	switch (block->type) {
	case FCSIM_BLOCK_STAT_RECT:
	case FCSIM_BLOCK_STAT_CIRC:
		*phys = stat_phys;
		break;
	case FCSIM_BLOCK_DYN_RECT:
	case FCSIM_BLOCK_DYN_CIRC:
		*phys = dyn_phys;
		break;
	case FCSIM_BLOCK_GOAL_RECT:
	case FCSIM_BLOCK_GOAL_CIRC:
	case FCSIM_BLOCK_WHEEL:
	case FCSIM_BLOCK_CW_WHEEL:
	case FCSIM_BLOCK_CCW_WHEEL:
		*phys = player_phys;
		break;
	case FCSIM_BLOCK_ROD:
		*phys = rod_phys;
		break;
	case FCSIM_BLOCK_SOLID_ROD:
		*phys = solid_rod_phys;
		break;
	}
}

static struct body_data *get_player_block_data(struct fcsim_level *level, int id, struct joint_map *map)
{
	struct fcsim_joint joints[5];
	struct body_data *data;
	struct joint_map_entry *entry;
	int joint_cnt;
	int i;

	joint_cnt = fcsim_get_player_block_joints(level, id, joints);

	data = (struct body_data *)malloc(sizeof(*data));
	data->joint_cnt = joint_cnt;

	for (i = 0; i < joint_cnt; i++) {
		entry = joint_map_find(map, &joints[i]);
		data->joint_ids[i] = entry->joint_id;
	}

	return data;
}

static void generate_player_block_body(struct fcsim_simul *simul, struct fcsim_level *level, int id, struct joint_map *map)
{
	struct fcsim_block *block = &level->player_blocks[id];
	struct fcsim_shape shape;
	struct fcsim_where where;
	struct block_physics phys;
	struct body_data *data;

	fcsim_get_player_block_desc(level, id, &shape, &where);
	get_block_phys(block, &phys);
	data = get_player_block_data(level, id, map);

	simul->player_block_bodies[id] = generate_body(&simul->world, &shape, &where, &phys, data);
}

static void generate_level_block_body(struct fcsim_simul *simul, struct fcsim_level *level, int id, struct joint_map *map)
{
	struct fcsim_block *block = &level->level_blocks[id];
	struct fcsim_shape shape;
	struct fcsim_where where;
	struct block_physics phys;
	struct body_data *data;

	fcsim_get_level_block_desc(level, id, &shape, &where);
	get_block_phys(block, &phys);
	data = NULL;

	simul->level_block_bodies[id] = generate_body(&simul->world, &shape, &where, &phys, data);
}

static void add_player_block_joints(struct fcsim_level *level, int id, struct joint_map *map)
{
	struct fcsim_joint joints[5];
	int joint_cnt;
	int i;

	joint_cnt = fcsim_get_player_block_joints(level, id, joints);
	for (i = 0; i < joint_cnt; i++)
		joint_map_add(map, &joints[i], id);
}

static void generate_joints(struct fcsim_simul *simul, struct fcsim_level *level, struct body_list *bodies, struct fcsim_joint *joint)
{
	struct fcsim_block *block;
	double x, y;
	int speed;
	int first_speed = 0;

	fcsim_get_joint_pos(level, joint, &x, &y);

	if (joint->type == FCSIM_JOINT_FREE) {
		block = &level->player_blocks[bodies->block_id];
		if (block->type == FCSIM_BLOCK_CW_WHEEL)
			first_speed = 5;
		if (block->type == FCSIM_BLOCK_CCW_WHEEL)
			first_speed = -5;
	}

	for (; bodies->next; bodies = bodies->next) {
		block = &level->player_blocks[bodies->next->block_id];
		speed = 0;
		if (block->type == FCSIM_BLOCK_CW_WHEEL)
			speed = 5;
		if (block->type == FCSIM_BLOCK_CCW_WHEEL)
			speed = -5;
		generate_joint(&simul->world,
			       simul->player_block_bodies[bodies->block_id],
			       simul->player_block_bodies[bodies->next->block_id],
			       x, y, speed - first_speed);
		first_speed = 0;
	}
}

static void generate_block_joints(struct fcsim_simul *simul, struct fcsim_level *level, int id, struct joint_map *map)
{
	struct fcsim_joint joints[5];
	struct joint_map_entry *entry;
	int joint_cnt;
	double x, y;
	int i;

	joint_cnt = fcsim_get_player_block_joints(level, id, joints);

	for (i = 0; i < joint_cnt; i++) {
		entry = joint_map_find(map, &joints[i]);
		if (entry->generated)
			continue;
		generate_joints(simul, level, entry->body_head, &entry->joint);
		entry->generated = 1;
	}
}

struct fcsim_simul *fcsim_make_simul(struct fcsim_level *level)
{
	struct fcsim_simul *simul;
	struct joint_map map;
	int i;

	simul = (struct fcsim_simul *)malloc(sizeof(*simul));
	if (!simul)
		return NULL;

	init_b2world(&simul->world);
	simul->player_block_cnt = level->player_block_cnt;
	simul->player_block_bodies = (b2Body **)malloc(level->player_block_cnt * sizeof(b2Body *));
	simul->level_block_cnt = level->level_block_cnt;
	simul->level_block_bodies = (b2Body **)malloc(level->level_block_cnt * sizeof(b2Body *));
	joint_map_init(&map);

	for (i = 0; i < level->player_block_cnt; i++)
		add_player_block_joints(level, i, &map);

	for (i = 0; i < level->player_block_cnt; i++)
		generate_player_block_body(simul, level, i, &map);

	for (i = 0; i < level->level_block_cnt; i++)
		generate_level_block_body(simul, level, i, &map);

	for (i = 0; i < level->player_block_cnt; i++)
		generate_block_joints(simul, level, i, &map);

	return simul;
}

void fcsim_step(struct fcsim_simul *simul)
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

void fcsim_get_level_block_desc_simul(struct fcsim_simul *simul, int id, struct fcsim_where *where)
{
	b2Body *body = simul->level_block_bodies[id];
	b2Vec2 pos;

	pos = body->GetOriginPosition();
	where->x = pos.x;
	where->y = pos.y;
	where->angle = body->GetRotation(); 
}

void fcsim_get_player_block_desc_simul(struct fcsim_simul *simul, int id, struct fcsim_where *where)
{
	b2Body *body = simul->player_block_bodies[id];
	b2Vec2 pos;

	pos = body->GetOriginPosition();
	where->x = pos.x;
	where->y = pos.y;
	where->angle = body->GetRotation(); 
}
