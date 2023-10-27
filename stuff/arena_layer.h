struct arena_view {
	float x;
	float y;
	float w_half;
	float h_half;
};

struct arena_layer {
	struct fcsim_arena arena;
	int loaded;

	struct button load_button;
	struct load_layer load_layer;

	struct runner *runner;
	int running;

	struct arena_view view;
	float view_scale;

	int prev_x;
	int prev_y;
	int pressed;

	int fast;
};

void arena_layer_init(struct arena_layer *arena_layer);
void arena_layer_show(struct arena_layer *arena_layer);
void arena_layer_draw(struct arena_layer *arena_layer);
void arena_layer_event(struct arena_layer *arena_layer, struct event *event);
