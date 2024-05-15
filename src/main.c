#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

void key_down(int key);
void key_up(int key);
void move(int x, int y);
void button_down(int button);
void button_up(int button);
void scroll(int delta);
void resize(int w, int h);
void init(void);
void draw(void);

void process_events(Display *dpy, Window win)
{
	XEvent xev;

	while (XPending(dpy) > 0) {
		XNextEvent(dpy, &xev);

		switch (xev.type) {
		case Expose:
			resize(xev.xexpose.width, xev.xexpose.height);
			break;
		case KeyPress:
			key_down(xev.xkey.keycode);
			break;
		case KeyRelease:
			key_up(xev.xkey.keycode);
			break;
		case ButtonPress:
			if (xev.xbutton.button == 4)
				scroll(+1);
			else if (xev.xbutton.button == 5)
				scroll(-1);
			else
				button_down(xev.xbutton.button);
			break;
		case ButtonRelease:
			if (xev.xbutton.button != 4 && xev.xbutton.button != 5)
				button_up(xev.xbutton.button);
			break;
		case MotionNotify:
			move(xev.xmotion.x, xev.xmotion.y);
			break;
		}
	}
}

int main(void)
{
	Display *dpy;
	Window root;
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;
	Window win;
	GLXContext glc;

	dpy = XOpenDisplay(NULL);
	if (!dpy)
		return 1;

	root = DefaultRootWindow(dpy);

	vi = glXChooseVisual(dpy, 0, att);
	if (!vi)
		return 1;

	cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

	swa.colormap = cmap;
	swa.event_mask = ExposureMask |
			 KeyPressMask |
			 KeyReleaseMask |
			 ButtonPressMask |
			 ButtonReleaseMask |
			 PointerMotionMask;

	win = XCreateWindow(dpy, root,
			    0, 0, 600, 600,
			    0,
			    vi->depth,
			    InputOutput,
			    vi->visual,
			    CWColormap | CWEventMask,
			    &swa);

	XMapWindow(dpy, win);

	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);

	init();

	while (1) {
		draw(); 
		glXSwapBuffers(dpy, win);
		process_events(dpy, win);
	}
}