struct arena_view {
	float x;
	float y;
	float w_half;
	float h_half;
};

enum drag_type {
	DRAG_NONE,
	DRAG_PAN,
	DRAG_MOVE_JOINT,
};

struct drag_action {
	enum drag_type type;
	int vertex_id;
};

struct arena_layer {
	struct fcsim_level level;
	struct fcsim_shape *player_shapes;
	struct fcsim_shape *level_shapes;
	struct fcsim_where *player_wheres;
	struct fcsim_where *level_wheres;
	int loaded;

	struct button load_button;
	struct load_layer load_layer;

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
void arena_layer_event(struct arena_layer *arena_layer, struct event *event);
