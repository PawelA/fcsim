struct view {
	float x;
	float y;
	float scale;
	float width;
	float height;
};

enum action {
	ACTION_NONE,
	ACTION_PAN,
	ACTION_MOVE_JOINT,
	ACTION_NEW_ROD,
	ACTION_NEW_WHEEL,
	ACTION_MOVE,
};

enum tool {
	TOOL_MOVE,
	TOOL_ROD,
	TOOL_SOLID_ROD,
	TOOL_WHEEL,
	TOOL_CW_WHEEL,
	TOOL_CCW_WHEEL,
	TOOL_DELETE,
};

struct block_head {
	struct block_head *next;
	struct block *block;
};

struct joint_head {
	struct joint_head *next;
	struct joint *joint;
	double orig_x;
	double orig_y;
};

struct block_graphics {
	unsigned short *indices;
	float *coords;
	float *colors;

	GLuint index_buffer;
	GLuint coord_buffer;
	GLuint color_buffer;

	int triangle_cnt;
	int vertex_cnt;
};

struct arena {
	struct design design;
	b2World *world;

	bool running;
	int ival;

	struct view view;

	int cursor_x;
	int cursor_y;
	bool shift;
	bool ctrl;

	enum tool tool;
	enum tool tool_hidden;
	enum action action;
	struct joint *hover_joint;
	struct block *hover_block;

	struct joint_head *root_joints_moving;
	struct block_head *root_blocks_moving;
	struct block_head *blocks_moving;
	float move_orig_x;
	float move_orig_y;

	struct block *new_block;

	struct block_graphics block_graphics;

	float joint_coords[48];
	GLuint joint_coord_buffer;
};

bool arena_compile_shaders(void);
void arena_init(struct arena *arena, float w, float h);

void arena_show(struct arena *arena);
void arena_draw(struct arena *arena);

void arena_key_up_event(struct arena *arena, int key);
void arena_key_down_event(struct arena *arena, int key);
void arena_mouse_move_event(struct arena *arena, int x, int y);
void arena_scroll_event(struct arena *arena, int delta);
void arena_mouse_button_up_event(struct arena *arena, int button);
void arena_mouse_button_down_event(struct arena *arena, int button);
void arena_size_event(struct arena *arena, float w, float h);
