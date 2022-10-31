struct fcsim_world;

struct fcsim_body {
	float x, y;
	float angle;
};

struct fcsim_world *fcsim_create_world(void);

void fcsim_add_stat_rect(struct fcsim_world *world,
			 struct fcsim_body *body,
			 float w, float h);

void fcsim_add_dyn_rect(struct fcsim_world *world,
			struct fcsim_body *body,
			float w, float h);

void fcsim_add_stat_circ(struct fcsim_world *world,
			 struct fcsim_body *body,
			 float r);

void fcsim_add_dyn_circ(struct fcsim_world *world,
			struct fcsim_body *body,
			float r);

void fcsim_step(struct fcsim_world *world);
