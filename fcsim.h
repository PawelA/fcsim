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
#define FCSIM_TYPE_MAX    10

struct fcsim_block {
	int type;
	float x, y;
	float w, h;
	float angle;
	int joints[2];
};

void fcsim_create_world(void);

void fcsim_add_block(fcsim_block *block);

void fcsim_step(void);
