#include <vector>
#include <stdio.h>

#include "box2d/Include/Box2D.h"

#include "fcsim.h"

#define ARENA_WIDTH	2000
#define ARENA_HEIGHT	1450

struct fcsim_link {
	struct fcsim_body *fc_body;
	b2Body *b2_body;
};

struct fcsim_world {
	b2World world;
	std::vector<fcsim_link> links;

	fcsim_world(b2AABB aabb, b2Vec2 gravity) :
		world(aabb, gravity, true) {}
};

struct fcsim_world *fcsim_create_world(void)
{
	b2AABB aabb;
	b2Vec2 gravity(0, 300);

	aabb.minVertex.Set(-ARENA_WIDTH, -ARENA_HEIGHT);
	aabb.maxVertex.Set(ARENA_WIDTH, ARENA_HEIGHT);

	return new fcsim_world(aabb, gravity);
}

void fcsim_add_stat_rect(struct fcsim_world *world,
			 struct fcsim_body *body,
			 float w, float h)
{
	b2BoxDef box_def;
	b2BodyDef body_def;

	box_def.extents.Set(w/2, h/2);
	box_def.friction = 0.7;
	body_def.position.Set(body->x, body->y);
	body_def.rotation = body->angle;
	body_def.AddShape(&box_def);

	world->world.CreateBody(&body_def);
}

void fcsim_add_dyn_rect(struct fcsim_world *world,
			struct fcsim_body *body,
			float w, float h)
{
	b2BoxDef box_def;
	b2BodyDef body_def;
	b2Body *b2_body;
	struct fcsim_link link;

	box_def.extents.Set(w/2, h/2);
	box_def.density = 1.0f;
	box_def.friction = 0.7f;
	//box_def.restitution = 0.2f;
	body_def.position.Set(body->x, body->y);
	body_def.rotation = body->angle;
	body_def.AddShape(&box_def);

	b2_body = world->world.CreateBody(&body_def);

	link.fc_body = body;
	link.b2_body = b2_body;
	world->links.push_back(link);
}

void fcsim_add_stat_circ(struct fcsim_world *world,
			 struct fcsim_body *body,
			 float r)
{
	b2CircleDef circle_def;
	b2BodyDef body_def;

	circle_def.radius = r;
	circle_def.friction = 0.7f;
	body_def.position.Set(body->x, body->y);
	body_def.rotation = body->angle;
	body_def.AddShape(&circle_def);

	world->world.CreateBody(&body_def);
}

void fcsim_add_dyn_circ(struct fcsim_world *world,
			struct fcsim_body *body,
			float r)
{
	b2CircleDef circle_def;
	b2BodyDef body_def;
	b2Body *b2_body;
	struct fcsim_link link;

	circle_def.radius = r;
	circle_def.density = 1.0f;
	circle_def.friction = 0.7f;
	//circle_def.restitution = 0.2f;
	body_def.position.Set(body->x, body->y);
	body_def.rotation = body->angle;
	body_def.AddShape(&circle_def);

	b2_body = world->world.CreateBody(&body_def);
	
	link.fc_body = body;
	link.b2_body = b2_body;
	world->links.push_back(link);
}

float aa = 0;
void fcsim_step(struct fcsim_world *world)
{
	world->world.Step(0.0333333f, 10);

	for (auto link : world->links) {
		b2Vec2 pos = link.b2_body->GetOriginPosition();
		float angle = link.b2_body->GetRotation();
		printf("%f %f %f\n", pos.x, pos.y, angle);

		link.fc_body->x = pos.x;
		link.fc_body->y = pos.y;
		link.fc_body->angle = angle;
	}
}
