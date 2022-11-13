#include "fcsim.h"
#include "draw.h"
#include "glwindow.h"

#include "level.h"

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
