struct arena_view {
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

struct arena_layer {
	struct fcsim_level level;
	struct fcsim_shape *player_shapes;
	struct fcsim_shape *level_shapes;
	struct fcsim_where *player_wheres;
	struct fcsim_where *level_wheres;

	struct runner *runner;
	int running;

	struct arena_view view;
	float view_scale;

	int prev_x;
	int prev_y;

	int fast;

	struct drag_action drag;
};

void arena_layer_init(struct arena_layer *arena_layer);
void arena_layer_show(struct arena_layer *arena_layer);
void arena_layer_draw(struct arena_layer *arena_layer);

void arena_layer_key_event(struct arena_layer *arena_layer, int key, int action);
void arena_layer_mouse_move_event(struct arena_layer *arena_layer);
void arena_layer_scroll_event(struct arena_layer *arena_layer, int delta);
void arena_layer_mouse_button_event(struct arena_layer *arena_layer, int button, int action);
void arena_layer_size_event(struct arena_layer *arena_layer);
