enum event_type {
	EVENT_KEY,
	EVENT_MOUSE_MOVE,
	EVENT_MOUSE_BUTTON,
	EVENT_SCROLL,
	EVENT_CHAR,
	EVENT_SIZE,
};

struct key_event {
	int key;
	int action;
};

struct mouse_button_event {
	int button;
	int action;
};

struct scroll_event {
	int delta;
};

struct char_event {
	unsigned int codepoint;
};

struct event {
	enum event_type type;
	union {
		struct key_event key_event;
		struct mouse_button_event mouse_button_event;
		struct scroll_event scroll_event;
		struct char_event char_event;
	};
};

