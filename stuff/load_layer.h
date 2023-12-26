struct load_layer {
	int open;
	int loading;
	char buf[17];
	struct input_box input;
	struct loader *loader;
	struct fcsimn_level level;
	int loaded;
};

void load_layer_init(struct load_layer *load_layer);
void load_layer_draw(struct load_layer *load_layer);
void load_layer_event(struct load_layer *load_layer, struct event *event);
