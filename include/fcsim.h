#ifndef __FCSIMN_H__
#define __FCSIMN_H__

struct fcsim_vertex {
	double x;
	double y;
};

enum fcsim_joint_type {
	FCSIMN_JOINT_FREE,
	FCSIMN_JOINT_DERIVED,
};

struct fcsim_free_joint {
	int vertex_id;
};

struct fcsim_derived_joint {
	int block_id;
	int index;
};

struct fcsim_joint {
	enum fcsim_joint_type type;
	union {
		struct fcsim_free_joint free;
		struct fcsim_derived_joint derived;
	};
};

enum fcsim_block_type {
	FCSIMN_BLOCK_STAT_RECT,
	FCSIMN_BLOCK_DYN_RECT,
	FCSIMN_BLOCK_STAT_CIRC,
	FCSIMN_BLOCK_DYN_CIRC,
	FCSIMN_BLOCK_GOAL_RECT,
	FCSIMN_BLOCK_GOAL_CIRC,
	FCSIMN_BLOCK_WHEEL,
	FCSIMN_BLOCK_CW_WHEEL,
	FCSIMN_BLOCK_CCW_WHEEL,
	FCSIMN_BLOCK_ROD,
	FCSIMN_BLOCK_SOLID_ROD,
};

struct fcsim_rod {
	struct fcsim_joint from;
	struct fcsim_joint to;
};

struct fcsim_wheel {
	struct fcsim_joint center;
	double radius;
	double angle;
};

struct fcsim_jrect {
	double x, y;
	double w, h;
	double angle;
};

struct fcsim_circ {
	double x, y;
	double radius;
};

struct fcsim_rect {
	double x, y;
	double w, h;
	double angle;
};

struct fcsim_block {
	enum fcsim_block_type type;
	union {
		struct fcsim_rod   rod;
		struct fcsim_wheel wheel;
		struct fcsim_jrect jrect;
		struct fcsim_circ  circ;
		struct fcsim_rect  rect;
	};
};

struct fcsim_area {
	double x, y;
	double w, h;
};

struct fcsim_level {
	struct fcsim_block *level_blocks;
	int level_block_cnt;

	struct fcsim_block *player_blocks;
	int player_block_cnt;

	struct fcsim_vertex *vertices;
	int vertex_cnt;

	struct fcsim_area build_area;
	struct fcsim_area goal_area;
};

struct fcsim_simul;

enum fcsim_shape_type {
	FCSIMN_SHAPE_CIRC,
	FCSIMN_SHAPE_RECT,
};

struct fcsim_shape_circ {
	double radius;
};

struct fcsim_shape_rect {
	double w, h;
};

struct fcsim_shape {
	enum fcsim_shape_type type;
	union {
		struct fcsim_shape_circ circ;
		struct fcsim_shape_rect rect;
	};
};

struct fcsim_where {
	double x, y;
	double angle;
};

#ifdef __cplusplus
extern "C" {
#endif

int fcsim_parse_xml(char *xml, int len, struct fcsim_level *level);

struct fcsim_simul *fcsim_make_simul(struct fcsim_level *level);

void fcsim_step(struct fcsim_simul *simul);

void fcsim_get_level_block_desc(struct fcsim_level *level, int id, struct fcsim_shape *shape, struct fcsim_where *where);
void fcsim_get_player_block_desc(struct fcsim_level *level, int id, struct fcsim_shape *shape, struct fcsim_where *where);

void fcsim_get_level_block_desc_simul(struct fcsim_simul *simul, int id, struct fcsim_where *where);
void fcsim_get_player_block_desc_simul(struct fcsim_simul *simul, int id, struct fcsim_where *where);

int fcsim_get_player_block_joints(struct fcsim_level *level, int id, struct fcsim_joint *joints);

void fcsim_get_joint_pos(struct fcsim_level *level, struct fcsim_joint *joint, double *x, double *y);

#ifdef __cplusplus
};
#endif

#endif
