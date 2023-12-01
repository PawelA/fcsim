int joint_cnt(struct block *block)
{
	switch (block->type) {
	case BLOCK_STAT_RECT:
	case BLOCK_STAT_CIRC:
	case BLOCK_DYN_RECT:
	case BLOCK_DYN_CIRC:
		return 0;
	case BLOCK_GOAL_RECT:
		return 5;
	case BLOCK_GOAL_CIRC:
	case BLOCK_WHEEL:
	case BLOCK_CW_WHEEL:
	case BLOCK_CCW_WHEEL:
		return 5;
	case BLOCK_ROD:
	case BLOCK_SOLID_ROD:
		return 2;
	}
}

void get_joint_position(struct level *level, 

void get_rod_joint_positions(struct level *level, struct rod *rod, double *coords)
{
	
}

void get_jrect_joint_positions(struct jrect *jrect, double *coords)
{
	double x = jrect->x;
	double y = jrect->y;
	double w_half = jrect->w/2;
	double h_half = jrect->h/2;

	double x0 =  fcsim_cos(jrect->angle) * w_half;
	double y0 =  fcsim_sin(jrect->angle) * w_half;
	double x1 =  fcsim_sin(jrect->angle) * h_half;
	double y1 = -fcsim_cos(jrect->angle) * h_half;

	coords[0] = x;
	coords[1] = y;
	coords[2] = x + x0 + x1;
	coords[3] = y + y0 + y1;
	coords[4] = x - x0 + x1;
	coords[5] = y - y0 + y1;
	coords[6] = x + x0 - x1;
	coords[7] = y + y0 - y1;
	coords[8] = x - x0 - x1;
	coords[9] = y - y0 - y1;
}

int get_derived_joint_pos(struct block *block, int index, double *x, double *y)
{
	switch (block->type) {
	case BLOCK_STAT_RECT:
	case BLOCK_STAT_CIRC:
	case BLOCK_DYN_RECT:
	case BLOCK_DYN_CIRC:
		return -1;
	case BLOCK_GOAL_RECT:
		return get_jrect_derived_joint_pos(&block->jrect, index, x, y);
	case BLOCK_GOAL_CIRC:
	case BLOCK_WHEEL:
	case BLOCK_CW_WHEEL:
	case BLOCK_CCW_WHEEL:
		return get_wheel_derived_joint_pos(&block->wheel, index, x, y);
	case BLOCK_ROD:
	case BLOCK_SOLID_ROD:
		return get_rod_derived_joint_pos(&block->wheel, index, x, y);
	}
}

static struct xml_block *find_id(struct xml_level *xml_level, int id)
{
	struct xml_block *b;

	for (b = xml_level->player_blocks; b; b = b->nex) {
		if (b->id == id)
			return id;
	}

	return NULL;
}

static int find_closest_joint(struct xml_level *xml_level, double x, double y,
		      struct derived_joint *res_joint)
{
	double best_dist = 10.0;
	struct xml_joint *j;
	struct xml_block *block;

	for (j = xml_block->joints; j; j = j->next) {
		block = find_id(xml_level, j->id);
		if (!block)
			continue;
	}
}

static void conv_block(struct xml_level *xml_level,
		       struct xml_block *xml_block, 
		       struct block *block)
{

		
}

}
void xml_conv(struct xml_level *xml_level, struct level *level)
{
	
}
