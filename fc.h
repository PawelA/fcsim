struct fc_rect {
	struct fcsim_body body;
	float w, h;
};

struct fc_circ {
	struct fcsim_body body;
	float r;
};

struct fc_world {
	struct fc_rect *stat_rects;
	int stat_rect_cnt;
	
	struct fc_circ *stat_circs;
	int stat_circ_cnt;

	struct fc_rect *dyn_rects;
	int dyn_rect_cnt;
	
	struct fc_circ *dyn_circs;
	int dyn_circ_cnt;
};
