#ifndef __FCSIM_H__
#define __FCSIM_H__

enum {
	XML_STATIC_RECTANGLE,
	XML_STATIC_CIRCLE,
	XML_DYNAMIC_RECTANGLE,
	XML_DYNAMIC_CIRCLE,
	XML_JOINTED_DYNAMIC_RECTANGLE,
	XML_NO_SPIN_WHEEL,
	XML_CLOCKWISE_WHEEL,
	XML_COUNTER_CLOCKWISE_WHEEL,
	XML_SOLID_ROD,
	XML_HOLLOW_ROD,
};

struct xml_position {
	double x;
	double y;
};

struct xml_joint {
	int id;
	struct xml_joint *next;
};

struct xml_block {
	int type;
	int id;
	double rotation;
	struct xml_position position;
	double width;
	double height;
	int goal_block;
	struct xml_joint *joints;

	struct xml_block *next;
};

struct xml_zone {
	struct xml_position position;
	double width;
	double height;
};

struct xml_level {
	struct xml_block *level_blocks;
	struct xml_block *player_blocks;
	struct xml_zone start;
	struct xml_zone end;
};

enum joint_type {
	JOINT_FREE,
	JOINT_DERIVED,
};

struct free_joint {
	double x;
	double y;
};

struct derived_joint {
	struct block *block;
	int index;
};

struct joint {
	enum joint_type type;
	union {
		struct free_joint free;
		struct derived_joint derived;
	};
};

enum block_type {
	BLOCK_STAT_RECT,
	BLOCK_STAT_CIRCLE,
	BLOCK_DYN_RECT,
	BLOCK_DYN_CIRCLE,
	BLOCK_GOAL_RECT,
	BLOCK_GOAL_CIRCLE,
	BLOCK_WHEEL,
	BLOCK_CW_WHEEL,
	BLOCK_CCW_WHEEL,
	BLOCK_ROD,
	BLOCK_SOLID_ROD,
};

struct rod {
	struct joint *from;
	struct joint *to;
};

struct wheel {
	struct joint *center;
	double radius;
	double angle;
};

struct jointed_rect {
	struct joint *center;
	double w, h;
	double angle;
};

struct circle {
	double x, y;
	double radius;
	double angle;
};

struct rectangle {
	double x, y;
	double w, h;
	double angle;
};

struct block {
	enum block_type type;
	union {
		struct rod rod;
		struct wheel wheel;
		struct jointed_rect jointed_rect;
		struct cicle circle;
		struct rectangle rectangle;
	};
};

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

int xml_parse(char *xml, int len, struct xml_level *level);

#ifdef __cplusplus
}
#endif

#endif
