struct view {
	float x;
	float y;
	float w_half;
	float h_half;
};

enum drag_type {
	DRAG_NONE,
	DRAG_PAN,
	DRAG_MOVE_VERTEX,
	DRAG_MOVE_BLOCK,
};

struct drag_action {
	enum drag_type type;
	int vertex_id;
	int block_id;
};

struct arena {
	struct design design;
	b2World *world;

	bool running;
	int ival;

	float width, height;
	struct view view;
	float view_scale;

	int prev_x;
	int prev_y;

	int fast;

	struct drag_action drag;
};

void arena_init(struct arena *arena, float w, float h);
void arena_show(struct arena *arena);
void arena_draw(struct arena *arena);

void arena_key_up_event(struct arena *arena, int key);
void arena_key_down_event(struct arena *arena, int key);
void arena_mouse_move_event(struct arena *arena);
void arena_scroll_event(struct arena *arena, int delta);
void arena_mouse_button_up_event(struct arena *arena, int button);
void arena_mouse_button_down_event(struct arena *arena, int button);
void arena_size_event(struct arena *arena, float w, float h);
