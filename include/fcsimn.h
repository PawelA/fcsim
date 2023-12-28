#ifndef __FCSIMN_H__
#define __FCSIMN_H__

struct fcsimn_vertex {
	double x;
	double y;
};

enum fcsimn_joint_type {
	FCSIMN_JOINT_FREE,
	FCSIMN_JOINT_DERIVED,
};

struct fcsimn_free_joint {
	int vertex_id;
};

struct fcsimn_derived_joint {
	int block_id;
	int index;
};

struct fcsimn_joint {
	enum fcsimn_joint_type type;
	union {
		struct fcsimn_free_joint free;
		struct fcsimn_derived_joint derived;
	};
};

enum fcsimn_block_type {
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

struct fcsimn_rod {
	struct fcsimn_joint from;
	struct fcsimn_joint to;
};

struct fcsimn_wheel {
	struct fcsimn_joint center;
	double radius;
	double angle;
};

struct fcsimn_jrect {
	double x, y;
	double w, h;
	double angle;
};

struct fcsimn_circ {
	double x, y;
	double radius;
};

struct fcsimn_rect {
	double x, y;
	double w, h;
	double angle;
};

struct fcsimn_block {
	enum fcsimn_block_type type;
	union {
		struct fcsimn_rod   rod;
		struct fcsimn_wheel wheel;
		struct fcsimn_jrect jrect;
		struct fcsimn_circ  circ;
		struct fcsimn_rect  rect;
	};
	void *body;
};

struct fcsimn_level {
	struct fcsimn_block *level_blocks;
	int level_block_cnt;

	struct fcsimn_block *player_blocks;
	int player_block_cnt;

	struct fcsimn_vertex *vertices;
	int vertex_cnt;
};

struct fcsimn_simul;

enum fcsimn_shape_type {
	FCSIMN_SHAPE_CIRC,
	FCSIMN_SHAPE_RECT,
};

struct fcsimn_shape_circ {
	double radius;
};

struct fcsimn_shape_rect {
	double w, h;
};

struct fcsimn_shape {
	enum shape_type type;
	union {
		struct fcsimn_shape_circ circ;
		struct fcsimn_shape_rect rect;
	};
};

struct fcsimn_where {
	double x, y;
	double angle;
};

struct 

#ifdef __cplusplus
extern "C" {
#endif

void fcsimn_get_joint_pos(struct fcsimn_level *level,
			  struct fcsimn_joint *joint,
			  double *x, double *y);

int fcsimn_parse_xml(char *xml, int len, struct fcsimn_level *level);

struct fcsimn_simul *fcsimn_make_simul(struct fcsimn_level *level);

void fcsimn_step(struct fcsimn_simul *simul);

void fcsimn_get_block_desc(struct fcsimn_level *level,
			   struct fcsimn_block *block,
			   struct fcsimn_shape *shape,
			   struct fcsimn_where *where);

void fcsimn_get_block_desc_simul(struct fcsimn_block *block, struct fcsimn_where *where);

#ifdef __cplusplus
};
#endif

#endif
