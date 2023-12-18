/*
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
		
int get_jrect_joints(struct jcsimn_jrect *jrect, struct fcsimn_joint *joints)
{
	joints[0] = jrect->center;
}

int get_joints(struct fcsimn_block *block, struct fcsimn_joint *joints)
{
	switch (block->type) {
	case BLOCK_GOAL_RECT:
		return get_jrect_joints(&block->jrect, joints);
	case BLOCK_GOAL_CIRC:
	case BLOCK_WHEEL:
	case BLOCK_CW_WHEEL:
	case BLOCK_CCW_WHEEL:
		return get_wheel_joints(&block->wheel, joints);
	case BLOCK_ROD:
	case BLOCK_SOLID_ROD:
		return get_rod_joints(&block->rod, joints);
	}

	return -1;
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

static double closest_joint_rod(struct fcsimn_rod *rod, double x, double y, struct fcsimn_joint *joint)
{
	
}

static double closest_joint_block(struct fcsimn_block *block, double x, double y, struct fcsimn_joint *joint)
{
	switch (block->type) {
	case FCSIMN_BLOCK_ROD:
	case FCSIMN_BLOCK_SOLID_ROD:
		return closest_joint_rod(&block->rod, x, y, joint);
	case FCSIMN_BLOCK_GOAL_CIRC:
	case FCSIMN_BLOCK_WHEEL:
	case FCSIMN_BLOCK_CW_WHEEL:
	case FCSIMN_BLOCK_CCW_WHEEL:
		return closest_joint_wheel(&block->wheel, x, y, joint);
	case FCSIMN_BLOCK_GOAL_RECT:
		return closest_joint_jrect(&block->jrect, x, y, joint);
	}

	return 0.0;
}

static int find_closest_joint(struct fcsimn_level *level, double x, double y,
			      struct xml_joint *block_ids,
			      struct fcsimn_joint *joint)
{
	double best_dist = 10.0;
	struct fcsimn_block *block;
	struct xml_joint *bid;
	int found = 0;

	for (bid = block_ids; bid; bid = bid->next) {
		if (bid->id >= level->player_block_cnt)
			continue;
		block = &level->player_blocks[j->id];
	}
}
*/

static int add_circ(struct fcsimn_level *level, struct xml_block *xml_block)
{
	struct fcsimn_block *block;

	block = level->level_blocks[level->level_block_cnt];
	level->level_block_cnt++;

	switch (xml_block->type) {
	case XML_STATIC_CIRCLE:
		block->type = FCSIMN_BLOCK_STAT_CIRC;
		break;
	case XML_DYNAMIC_CIRCLE:
		block->type = FCSIMN_BLOCK_DYN_CIRC;
		break;
	default:
		return -1;
	}

	block->circ.x = xml_block->position.x;
	block->circ.y = xml_block->position.y;
	block->circ.radius = xml_block->w;

	return 0;
}

static int add_rect(struct fcsimn_level *level, struct xml_block *block)
{
	struct fcsimn_block *block;

	block = level->level_blocks[level->level_block_cnt];
	level->level_block_cnt++;

	switch (xml_block->type) {
	case XML_STATIC_RECTANGLE:
		block->type = FCSIMN_BLOCK_STAT_RECT;
		break;
	case XML_DYNAMIC_RECTANGLE:
		block->type = FCSIMN_BLOCK_DYN_RECT;
		break;
	default:
		return -1;
	}

	block->rect.x = xml_block->position.x;
	block->rect.y = xml_block->position.y;
	block->rect.w = xml_block->width;
	block->rect.h = xml_block->height;
	block->rect.angle = xml_block->rotation;

	return 0;
}

static int add_jrect(struct fcsimn_level *level, struct xml_block *block)
{
	return 0;
}

static int add_wheel(struct fcsimn_level *level, struct xml_block *block)
{
	return 0;
}

static int add_rod(struct fcsimn_level *level, struct xml_block *block)
{
	return 0;
}

static int add_level_block(struct fcsimn_level *level, struct xml_block *block)
{
	switch (block->type) {
	case XML_STATIC_CIRCLE:
	case XML_DYNAMIC_CIRCLE:
		return add_circ(level, block);
	case XML_STATIC_RECTANGLE:
	case XML_DYNAMIC_RECTANGLE:
		return add_rect(level, block);
	}

	return -1;
}

static int add_player_block(struct fcsimn_level *level, struct xml_block *block)
{
	switch (block->type) {
	case XML_JOINTED_DYNAMIC_RECTANGLE:
		return add_jrect(level, block);
	case XML_NO_SPIN_WHEEL:
	case XML_CLOCKWISE_WHEEL:
	case XML_COUNTER_CLOCKWISE_WHEEL:
		return add_wheel(level, block);
	case XML_SOLID_ROD:
	case XML_HOLLOW_ROD:
		return add_rod(level, block);
	}

	return -1;
}

static int get_block_count(struct xml_block *blocks)
{
	int len = 0;

	while (blocks) {
		len++;
		blocks = blocks->next;
	}

	return len;
}

static int convert_level(struct xml_level *xml_level, struct fcsimn_level *level)
{
	struct xml_block *block;
	int level_block_cnt;
	int player_block_cnt;
	int vertex_cnt;

	level_block_cnt  = get_block_count(xml_level->level_blocks);
	player_block_cnt = get_block_count(xml_level->player_blocks);
	vertex_cnt = player_block_cnt * 2;

	level->level_blocks = calloc(level_block_cnt, sizeof(struct fcsimn_block));
	if (!level->level_blocks)
		goto err_level_blocks;

	level->player_blocks = calloc(player_block_cnt, sizeof(struct fcsimn_block));
	if (!level->level_blocks)
		goto err_player_blocks;

	level->vertices = calloc(vertex_cnt, sizeof(struct fcsimn_vertex));
	if (!level->vertices)
		goto err_vertices;

	for (block = xml_level->level_blocks; block; block = block->next) {
		res = add_level_block(level, block);
		if (res)
			goto err;
	}

	for (block = xml_level->player_blocks; block; block = block->next) {
		res = add_player_block(level, block);
		if (res)
			goto err;
	}

	return 0;

err:
	free(level->vertices);
err_vertices:
	free(level->player_blocks);
err_player_blocks:
	free(level->level_blocks);
err_level_blocks:

	return -1;
}

int fcsimn_parse_xml(char *xml, int len, struct fcsimn_level *level)
{
	struct xml_level xml_level;
	int res;

	res = xml_parse(xml, len, &xml_level);
	if (res)
		return res;

	res = convert_level(&xml_level, level);
	if (res)
		return res;

	return 0;
}
