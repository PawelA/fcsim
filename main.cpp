#include "fcsim.h"
#include "draw.h"
#include "glwindow.h"

#if 0
struct fcsim_block blocks[] = {
	/*       type          x     y    w    h    a      j0  j1  */
	{ FCSIM_STAT_RECT  ,  150,    0, 300, 100,  0  , { -1, -1 } },
	{ FCSIM_STAT_RECT  , -200, -120, 200, 100,  0.2, { -1, -1 } },
	{ FCSIM_STAT_CIRCLE, -350, -300,  40,  40,  0  , { -1, -1 } },
	{ FCSIM_DYN_RECT   , -300, -400,  80,  80, -0.1, { -1, -1 } },
	{ FCSIM_DYN_CIRCLE , -270, -270,  30,  30,  0  , { -1, -1 } },
	{ FCSIM_WHEEL      ,  100, -200,  40,  40,  0  , { -1, -1 } },
	{ FCSIM_CW_WHEEL   ,  200, -200,  40,  40,  0  , { -1, -1 } },
	{ FCSIM_CCW_WHEEL  ,  300, -200,  40,  40,  0  , { -1, -1 } },
	{ FCSIM_ROD        ,  150, -200, 100,   0,  0  , { -1, -1 } },
	{ FCSIM_SOLID_ROD  ,  250, -200, 100,   0,  0  , { -1, -1 } },
};
#else
#include "level.h"
#endif

void add_world(struct fcsim_world *world)
{
	struct fcsim_block *block;

	for (block = blocks; block->type; block++)
		fcsim_add_block(block);
}

int main(void)
{
	int block_count = sizeof(blocks) / sizeof(blocks[0]);
	int f = 0;
	int i;

	glwindow_create(800, 800);

	fcsim_create_world();
	for (i = 0; i < block_count; i++)
		fcsim_add_block(&blocks[i]);

	setup_draw();
	while (1) {
		draw_world(blocks, block_count);
		glwindow_get_event();
		glwindow_swap_buffers();
		//if (f++ % 4 == 0)
			fcsim_step();
	}

	return 0;
}
