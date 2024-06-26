struct attach_node {
	struct attach_node *prev;
	struct attach_node *next;
	struct block *block;
};

struct attach_list {
	struct attach_list *head;
	struct attach_list *tail;
};

struct joint {
	struct block *gen;
	double x, y;
	struct attach_list *att;
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
#define WATER_COLLITION_BIT	(1 << 2)

#define ENV_COLLISION_MASK	(ENV_COLLISION_BIT | SOLID_COLLISION_BIT | WATER_COLLISION_BIT)
#define SOLID_COLLISION_MASK	(ENV_COLLISION_BIT | SOLID_COLLISION_BIT)
#define WATER_COLLITION_MASK	(ENV_COLLISION_BIT)

struct material {
	double density;
	double friction;
	double restitution;
	double linear_damping;
	double angular_damping;
	uint32_t collision_bit;
	uint32_t collision_mask;
};

struct block {
	struct block *prev;
	struct block *next;
	struct shape shape;
	struct material material;
	bool goal;
};

struct block_list {
	struct block *head;
	struct block *tail;
};

struct design {
	struct block_list level_blocks;
	struct block_list player_blocks;
};
