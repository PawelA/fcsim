#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdbool.h>

bool running = true;

LONG WINAPI WindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{ 
	switch(msg) {
	case WM_CLOSE:
		running = false;
		return 0;
	}

	return DefWindowProc(window, msg, wparam, lparam);
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

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	while (running) {
		MSG msg;

		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(dc);
		while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}
