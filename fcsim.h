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

struct fcsim_block_def {
	int type;
	double x, y;
	double w, h;
	double angle;
	int id;
	int joints[2];
};

void fcsim_create_world(void);

void fcsim_add_block(fcsim_block_def *bdef);

void fcsim_generate(void);

void fcsim_step(void);

int fcsim_parse_xml(const char *xml, fcsim_block_def *bdefs);
