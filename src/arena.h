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
};

enum tool {
	TOOL_MOVE,
	TOOL_ROD,
	TOOL_SOLID_ROD,
	TOOL_DELETE,
};

struct new_rod {
	struct joint *j0;
	double x0, y0;
	struct joint *j1;
	double x1, y1;
	bool solid;
};

struct arena {
	struct design design;
	b2World *world;

	bool running;
	int ival;

	struct view view;

	int cursor_x;
	int cursor_y;

	enum tool tool;
	enum action action;
	struct joint *hover_joint;

	struct new_rod new_rod;

	unsigned short indices[2048];
	float coords[2048];
	float colors[2048];

	int cnt_index;
	int cnt_coord;
	int cnt_color;

	GLuint index_buffer;
	GLuint coord_buffer;
	GLuint color_buffer;

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
