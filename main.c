#include <windows.h>
#include "gl.h"
#include <stdio.h>
#include <stdbool.h>

bool running = true;

LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{ 
	switch(msg) {
	case WM_CLOSE:
		running = false;
		return 0;
	}

	return DefWindowProc(window, msg, wparam, lparam);
} 

void gl_load(void *(*load)(const char *));
void *win32_load(const char *str);

const GLchar *vertex_shader_src =
	"attribute vec2 a_coords;"
	"void main() {"
		"gl_Position = vec4(a_coords, 0.0, 1.0);"
	"}";

const GLchar *fragment_shader_src =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"#endif\n"
	"void main() {"
		"gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"}\n";

static GLfloat coords[] = {
	0.6, 0.6,
	-0.6, -0.6,
	0.0, 1.0,
};

GLuint program;
GLuint coord_buffer;

void gl_init(void)
{
	GLchar shader_log[1024];
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLint param;
	GLsizei log_len;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &param);
	if (!param) {
		glGetShaderInfoLog(vertex_shader, sizeof(shader_log), &log_len, shader_log);
		printf("vertex shader:\n%s\n", shader_log);
		exit(1);
	}

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &param);
	if (!param) {
		glGetShaderInfoLog(fragment_shader, sizeof(shader_log), &log_len, shader_log);
		printf("fragment shader:\n%s", shader_log);
		exit(1);
	}

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glUseProgram(program);

	glGenBuffers(1, &coord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev_inst, LPSTR cmd_line, int cmd_show)
{
	WNDCLASSA wc;
	HWND window;
	HDC dc;
	PIXELFORMATDESCRIPTOR pfd;
	int pf;

	memset(&wc, 0, sizeof(wc));
	wc.style         = CS_OWNDC;
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = inst;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "fcsim";

	RegisterClassA(&wc);

	window = CreateWindowA("fcsim", "fcsim", WS_OVERLAPPEDWINDOW |
			WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, 0, 256, 256, NULL, NULL, inst, NULL);

	dc = GetDC(window);

	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	pf = ChoosePixelFormat(dc, &pfd);

	SetPixelFormat(dc, pf, &pfd);

	HGLRC glrc = wglCreateContext(dc);
	wglMakeCurrent(dc, glrc);

	ShowWindow(window, cmd_show);

	gl_load(win32_load);
	gl_init();
	glClearColor(0.5, 0.3, 1.0, 1.0);

	while (running) {
		MSG msg;

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		SwapBuffers(dc);
		while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}
