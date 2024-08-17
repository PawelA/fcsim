typedef struct b2Body b2Body;
struct b2Body;

typedef struct b2World b2World;
struct b2World;

struct attach_node {
	struct attach_node *prev;
	struct attach_node *next;
	struct block *block;
};

struct attach_list {
	struct attach_node *head;
	struct attach_node *tail;
};

struct joint {
	struct joint *prev;
	struct joint *next;
	struct block *gen;
	double x, y;
	struct attach_list att;
};

struct rect {
	double x, y;
	double w, h;
	double angle;
};

struct circ {
	double x, y;
	double radius;
};

struct box {
	double x, y;
	double w, h;
	double angle;

	struct joint *center;
	struct joint *corners[4];
};

struct rod {
	struct joint *from;
	struct attach_node *from_att;
	struct joint *to;
	struct attach_node *to_att;
	double width;
};

struct wheel {
	struct joint *center;
	struct attach_node *center_att;
	double radius;
	double angle;

	struct joint *spokes[4];
};

enum shape_type {
	SHAPE_RECT,
	SHAPE_CIRC,
	SHAPE_BOX,
	SHAPE_ROD,
	SHAPE_WHEEL,
};

struct shape {
	enum shape_type type;
	union {
		struct rect  rect;
		struct circ  circ;
		struct box   box;
		struct rod   rod;
		struct wheel wheel;
	};
};

#define ENV_COLLISION_BIT	(1 << 0)
#define SOLID_COLLISION_BIT	(1 << 1)
#define WATER_COLLISION_BIT	(1 << 2)

#define ENV_COLLISION_MASK	(ENV_COLLISION_BIT | SOLID_COLLISION_BIT | WATER_COLLISION_BIT)
#define SOLID_COLLISION_MASK	(ENV_COLLISION_BIT | SOLID_COLLISION_BIT)
#define WATER_COLLISION_MASK	(ENV_COLLISION_BIT)

struct material {
	double density;
	double friction;
	double restitution;
	double linear_damping;
	double angular_damping;
	uint32_t collision_bit;
	uint32_t collision_mask;
	float r;
	float g;
	float b;
};

struct block {
	struct block *prev;
	struct block *next;
	struct shape shape;
	struct material *material;
	bool goal;
	int id;
	b2Body *body;
};

struct joint_list {
	struct joint *head;
	struct joint *tail;
};

struct block_list {
	struct block *head;
	struct block *tail;
};

struct area {
	double x, y;
	double w, h;
};

struct design {
	struct joint_list joints;
	struct block_list level_blocks;
	struct block_list player_blocks;
	struct area build_area;
	struct area goal_area;
};

enum shell_type {
	SHELL_CIRC,
	SHELL_RECT,
};

struct shell_circ {
	double radius;
};

struct shell_rect {
	double w, h;
};

struct shell {
	enum shell_type type;
	union {
		struct shell_circ circ;
		struct shell_rect rect;
	};
	double x, y;
	double angle;
};

struct xml_level;

void convert_xml(struct xml_level *xml_level, struct design *design);
void free_design(struct design *design);

b2World *gen_world(struct design *design);
void step(struct b2World *world);
void get_shell(struct shell *shell, struct shape *shape);
int get_block_joints(struct block *block, struct joint **res);
