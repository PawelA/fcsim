struct input_box {
	char *buf;
	int len;
	int pos;
};

void input_box_init(struct input_box *input, char *buf, int len);
void input_box_add_char(struct input_box *input, char c);
void input_box_backspace(struct input_box *input);
void input_box_move_cursor(struct input_box *input, int delta);

void draw_input_box(struct input_box *input, int x, int y, int scale);

enum button_state {
	BUTTON_NORMAL,
	BUTTON_HOVERED,
	BUTTON_PRESSED,
};

struct button {
	char *text;
	int len;
	int x;
	int y;
	enum button_state state;
};

void button_init(struct button *button, char *text, int x, int y);
int button_hit_test(struct button *button, int x, int y);

void draw_button(struct button *button);
