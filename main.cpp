#include <GLFW/glfw3.h>
#include "fcsim.h"
#include "draw.h"
#include "level.h"

int main(void)
{
	int block_count = sizeof(blocks) / sizeof(blocks[0]);
	GLFWwindow *window;

	if (!glfwInit())
		return 1;
	window = glfwCreateWindow(800, 800, "fcsim demo", NULL, NULL);
	if (!window)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	fcsim_create_world();
	for (int i = 0; i < block_count; i++)
		fcsim_add_block(&blocks[i]);

	setup_draw();
	while (!glfwWindowShouldClose(window)) {
		draw_world(blocks, block_count);
		glfwSwapBuffers(window);
		glfwPollEvents();
		fcsim_step();
	}

	return 0;
}
