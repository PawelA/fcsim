#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

static Display *dpy;
static Window win;
static int screen;

static GLint att[] = {
	GLX_RGBA,
	GLX_DEPTH_SIZE, 24,
	GLX_DOUBLEBUFFER,
	None
};

int glwindow_create(int width, int height)
{
	Window root;
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;
	GLXContext glc;

	dpy = XOpenDisplay(NULL);
	if (!dpy)
		return -1;

 	screen = XDefaultScreen(dpy);
 	root = XRootWindow(dpy, screen);

	vi = glXChooseVisual(dpy, 0, att);
	if (!vi)
		return -1;

	cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask;
 
	win = XCreateWindow(dpy, root,
			    0, 0, width, height, 0,
			    vi->depth,
			    InputOutput,
			    vi->visual,
			    CWColormap | CWEventMask,
			    &swa);

	XMapWindow(dpy, win);
 
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);

	return 0;
}

int glwindow_get_event(void)
{
	XEvent xev;

	while (XPending(dpy) > 0) {
		XNextEvent(dpy, &xev);
	}

	return 0;
}

void glwindow_swap_buffers(void)
{
	glXSwapBuffers(dpy, win);
}
