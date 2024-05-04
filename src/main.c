#include <GLFW/glfw3.h>

void key_down(int key);
void key_up(int key);
void move(int x, int y);
void button_down(int button);
void button_up(int button);
void scroll(int delta);
void resize(int w, int h);
void init(void);
void draw(void);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
		key_down(key);
	else
		key_up(key);
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
	move(x, y);
}

void scroll_callback(GLFWwindow *window, double x, double y)
{
	scroll(y);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
		button_down(button);
	else
		button_up(button);
}

void window_size_callback(GLFWwindow *window, int w, int h)
{
	resize(w, h);
}

int main(void)
{
	GLFWwindow *window;
	int res;

	if (!glfwInit())
		return 1;

	window = glfwCreateWindow(800, 800, "fcsim", NULL, NULL);
	if (!window)
		return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	init();

	while (!glfwWindowShouldClose(window)) {
		draw();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}
