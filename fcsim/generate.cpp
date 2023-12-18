struct fcsimn_simul {
	b2World world;
};

void init_b2world(b2World *world)
{
	b2Vec2 gravity;
	b2AABB aabb;

	gravity.Set(0, 300);
	aabb.minVertex.Set(-2000, -1450);
	aabb.maxVertex.Set(2000, 1450);
	new (world) b2World(aabb, gravity, true);
}

struct fcsimn_simul *fcsimn_make_simul(struct fcsimn_level *level)
{
	struct fcsimn_simul *simul;

	simul = (struct fcsim_simul *)malloc(sizeof(*simul));
	if (!simul)
		return NULL;

	init_b2world(&simul->world);
}

struct 
