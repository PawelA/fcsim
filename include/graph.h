struct adj_block_list_elem {
	struct adj_block_list_elem *prev;
	struct adj_block_list_elem *next;
	struct block *block;
};

struct adj_block_list {
	struct adj_block_list_elem *blocks_head;
	struct adj_block_list_elem *blocks_tail;
};

struct joint {
	bool generated;
	double x, y;
	struct adj_block_list blocks;
};

enum block_type {
	BLOCK_TYPE_ROD,
	BLOCK_TYPE_WHEEL,
	BLOCK_TYPE_RECT,
};

struct piece {
	enum piece_type type;
	union {
		struct rod   *rod;
		struct wheel *wheel;
		struct rect  *rect;
	};
	struct block *prev;
	struct block *next;
};

struct rod {
	struct joint *from;
	struct block_list_elem *from_elem;
	struct joint *to;
	struct block_list_elem *to_elem;
	double width;
};

struct wheel {
	struct joint *center;
	struct block_list_elem *center_elem;
	double radius;
	double angle;

	struct joint *spokes[4];
};

struct rect {
	double x, y;
	double w, h;
	double angle;

	struct joint *center;
	struct joint *corners[4];
};

struct level {
	struct 
};
