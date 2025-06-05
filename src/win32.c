#include <windows.h>
#include <windowsx.h>
#include "gl.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "poocs.h"

void key_down(int key);
void key_up(int key);
void move(int x, int y);
void button_down(int button);
void button_up(int button);
void scroll(int delta);
void resize(int w, int h);
void init(char *xml, int len);
void draw(void);

bool running = true;
bool initialized = false;

int map_key(unsigned int vk)
{
	switch (vk) {
	case VK_SPACE: return 65;
	case 'R': return 27;
	case 'M': return 58;
	case 'S': return 39;
	case 'D': return 40;
	case 'U': return 30;
	case 'W': return 25;
	case 'C': return 54;
	case '1': return 10;
	case '2': return 11;
	case '3': return 12;
	case '4': return 13;
	case VK_SHIFT: return 50;
	case VK_CONTROL: return 37;
	}

	return -1;
}

LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (!initialized)
		return DefWindowProc(window, msg, wparam, lparam);

	switch(msg) {
	case WM_KEYDOWN:
		key_down(map_key(wparam));
		return 0;
	case WM_KEYUP:
		key_up(map_key(wparam));
		return 0;
	case WM_MOUSEWHEEL:
		scroll(GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? 1 : -1);
		return 0;
	case WM_MOUSEMOVE:
		move(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;
	case WM_LBUTTONDOWN:
		button_down(1);
		return 0;
	case WM_LBUTTONUP:
		button_up(1);
		return 0;
	case WM_SIZE:
		resize(LOWORD(lparam), HIWORD(lparam));
		return 0;
	case WM_CLOSE:
		running = false;
		return 0;
	}

	return DefWindowProc(window, msg, wparam, lparam);
}

void gl_load(void *(*load)(const char *));
void *win32_load(const char *str);

struct slot {
	void (*func)(void *arg);
	void *arg;
	int delay;
	uint64_t next_ts;
};

struct slot slots[4];

uint64_t get_ts(void)
{
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);

	return ((uint64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime) / 10000;
}

CONDITION_VARIABLE cond;
CRITICAL_SECTION mutex;

DWORD WINAPI thread_func(void *arg)
{
	uint64_t this_ts;
	uint64_t next_ts;
	DWORD timeout = INFINITE;
	int i;

	EnterCriticalSection(&mutex);
	while (1) {
		SleepConditionVariableCS(&cond, &mutex, timeout);

		this_ts = get_ts();
		for (i = 0; i < 4; i++) {
			if (slots[i].func && slots[i].next_ts <= this_ts) {
				slots[i].func(slots[i].arg);
				slots[i].next_ts += slots[i].delay;
			}
		}
		next_ts = (uint64_t)-1;
		for (i = 0; i < 4; i++) {
			if (slots[i].func && slots[i].next_ts < next_ts)
				next_ts = slots[i].next_ts;
		}

		if (next_ts != (uint64_t)-1)
			timeout = next_ts > this_ts ? next_ts - this_ts : 0;
		else
			timeout = INFINITE;
	}

	return 0;
}

int set_interval(void (*func)(void *arg), int delay, void *arg)
{
	uint64_t this_ts;
	int i;

	for (i = 0; i < 4; i++) {
		if (!slots[i].func)
			break;
	}
	if (i == 4)
		return -1;

	this_ts = get_ts();

	slots[i].func = func;
	slots[i].arg = arg;
	slots[i].delay = delay;
	slots[i].next_ts = this_ts + delay;

	WakeConditionVariable(&cond);

	return i;
}

void clear_interval(int id)
{
	slots[id].func = NULL;
}


void swap_interval(int interval)
{
	BOOL (GL_APIENTRY *wglSwapIntervalEXT)(int);

	wglSwapIntervalEXT = (BOOL (GL_APIENTRY *)(int))wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(interval);
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
			0, 0, 800, 800, NULL, NULL, inst, NULL);

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

	InitializeConditionVariable(&cond);
	InitializeCriticalSection(&mutex);
	CreateThread(NULL, 0, thread_func, NULL, 0, NULL);

	init(poocs_xml, sizeof(poocs_xml));
	initialized = true;

	swap_interval(1);

	while (running) {
		MSG msg;

		EnterCriticalSection(&mutex);
		draw();
		LeaveCriticalSection(&mutex);

		SwapBuffers(dc);

		EnterCriticalSection(&mutex);
		while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		LeaveCriticalSection(&mutex);
	}

	return 0;
}
