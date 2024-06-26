struct runner;

struct runner_tick {
	struct fcsim_where *player_wheres;
	struct fcsim_where *level_wheres;
	uint64_t index;
};

struct runner *runner_create(void);

void runner_load(struct runner *runner, struct fcsim_level *level);

void runner_start(struct runner *runner);

void runner_stop(struct runner *runner);

void runner_get_tick(struct runner *runner, struct runner_tick *tick);

uint64_t runner_get_won_tick(struct runner *runner);

void runner_set_frame_limit(struct runner *runner, uint64_t frame_limit_us);
