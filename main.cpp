#include "fcsim.h"
#include "fc.h"
#include "draw.h"

struct fc_rect stat_rects[] = {
	{ { 100, 0, 0 }, 300, 100 },
	{ { -200, -120, 0.2 }, 200, 100 },
};

struct fc_circ stat_circs[] = {
	{ { -350, -300, 0 }, 40 },
};

struct fc_rect dyn_rects[] = {
	{ { -300, -400, -0.1 }, 80, 80 },
};

struct fc_circ dyn_circs[] = {
	{ { -270, -270, 0 }, 30 },
};

struct fc_world world = {
	.stat_rects = stat_rects,
	.stat_rect_cnt = 2,

	.stat_circs = stat_circs,
	.stat_circ_cnt = 1,

	.dyn_rects = dyn_rects,
	.dyn_rect_cnt = 1,
	
	.dyn_circs = dyn_circs,
	.dyn_circ_cnt = 1,
};

void add_world(struct fcsim_world *sim_world, struct fc_world *world)
{
	int i;

	for (i = 0; i < world->stat_rect_cnt; i++) {
		fcsim_add_stat_rect(sim_world,
				    &world->stat_rects[i].body,
				    world->stat_rects[i].w,
				    world->stat_rects[i].h);
	}
	
	for (i = 0; i < world->stat_circ_cnt; i++) {
		fcsim_add_stat_circ(sim_world,
				    &world->stat_circs[i].body,
				    world->stat_circs[i].r);
	}
	
	for (i = 0; i < world->dyn_rect_cnt; i++) {
		fcsim_add_dyn_rect(sim_world,
				   &world->dyn_rects[i].body,
				   world->dyn_rects[i].w,
				   world->dyn_rects[i].h);
	}
	
	for (i = 0; i < world->dyn_circ_cnt; i++) {
		fcsim_add_dyn_circ(sim_world,
				   &world->dyn_circs[i].body,
				   world->dyn_circs[i].r);
	}
}

int main(void)
{
	struct fcsim_world *sim_world;

	create_window(600, 600, "window");

	sim_world = fcsim_create_world();
	add_world(sim_world, &world);

	while (!window_should_close()) {
		draw_world(&world);
		fcsim_step(sim_world);
	}

	destroy_window();

	return 0;
}
