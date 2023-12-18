#ifndef __FCSIM_H__
#define __FCSIM_H__

#define FCSIM_STAT_RECT   0
#define FCSIM_STAT_CIRCLE 1
#define FCSIM_DYN_RECT    2
#define FCSIM_DYN_CIRCLE  3
#define FCSIM_GOAL_RECT   4
#define FCSIM_GOAL_CIRCLE 5
#define FCSIM_WHEEL       6
#define FCSIM_CW_WHEEL    7
#define FCSIM_CCW_WHEEL   8
#define FCSIM_ROD         9
#define FCSIM_SOLID_ROD   10

#define FCSIM_TYPE_LAST FCSIM_SOLID_ROD

struct fcsim_block_stat {
	double x, y;
	double angle;
};

struct fcsim_block_def {
	int type;
	int id;
	double x, y;
	double w, h;
	double angle;
	int joints[2];
	int joint_cnt;
};

struct fcsim_rect {
	double x, y;
	double w, h;
};

struct fcsim_arena {
	struct fcsim_block_def *blocks;
	int block_cnt;
	struct fcsim_rect build;
	struct fcsim_rect goal;
};

struct fcsim_handle;

#ifdef __cplusplus
extern "C" {
#endif

struct fcsim_handle *fcsim_new(struct fcsim_arena *arena);

void fcsim_step(struct fcsim_handle *handle);

void fcsim_get_block_stats(struct fcsim_handle *handle,
			   struct fcsim_block_stat *stats);

int fcsim_read_xml(char *xml, int len, struct fcsim_arena *arena);

int fcsim_has_won(struct fcsim_arena *arena, struct fcsim_block_stat *stats);

struct fcsim_arena *fcsim_arena_from_handle(struct fcsim_handle *handle);

#ifdef __cplusplus
}
#endif

#endif
