/*
 * http://code.arp242.net/find-cursor
 * Copyright © 2015 Martin Tournoij <martin@arp242.net>
 * See below for full copyright
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
 
#include <X11/Xlib.h>
#include <X11/Xutil.h>


// Some variables you can play with :-)
int size = 120;
int step = 40;
int speed = 4000;
int line_width = 5;
char color_name[] = "white";

typedef struct cursor_positiion_t {
	int  win_x;
	int  win_y;
	int  root_x;
	int  root_y;
} CursorPosition;

typedef struct circle_t {
	int     size;
	int     width;
	char    *color;
	XColor  _color;
} Circle;

typedef struct window_info_t {
	Window  window;
	int     size;
	GC      gc;
} WindowInfo;

int cursor_position_poll(CursorPosition *pos, Display *display, int screen) {
	Window child_win, root_win;
	unsigned int mask;

	return XQueryPointer(display, XRootWindow(display, screen),
			&child_win, &root_win,
			&(pos->root_x), &(pos->root_y),
			&(pos->win_x), &(pos->win_y),
	        &mask
		);
}



// Circle *circle_init(Display *display, Colormap colormap, int size, int width, char *color_name) {
// 	Circle *circle = malloc(sizeof(Circle));

// 	XAllocNamedColor(display, colormap, color_name, &(circle->color), &(circle->color));

// 	return circle;
// }


WindowInfo *window_init(Display *display, int screen, Circle *circles, int n_circles) {
	int size = -1;
	XVisualInfo vinfo;
    XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo);

    for (int i = 0; i < n_circles; i += 1) {
    	int _size = circles[i].size + circles[i].width;
    	if (_size > size) {
    		size = _size;
    	}
    }

	XSetWindowAttributes window_attr;
    window_attr.colormap = XCreateColormap(display, DefaultRootWindow(display), vinfo.visual, AllocNone);
    window_attr.border_pixel = 0;
    window_attr.background_pixel = 0;
    window_attr.override_redirect = 1;

	Window window = XCreateWindow(display, XRootWindow(display, screen),
			0, 0,
		    size, size,
			0,
			vinfo.depth,
			CopyFromParent,
			vinfo.visual,
			CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect,
			&window_attr
		);

	XMapWindow(display, window);
	XStoreName(display, window, "find-cursor");

	XClassHint *class = XAllocClassHint();
	class->res_name = "find-cursor";
	class->res_class = "find-cursor";
	XSetClassHint(display, window, class);
	XFree(class);

	// Keep the window on top
	XEvent e;
	memset(&e, 0, sizeof(e));
	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
	e.xclient.display = display;
	e.xclient.window = window;
	e.xclient.format = 32;
	e.xclient.data.l[0] = 1;
	e.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_STAYS_ON_TOP", False);
	XSendEvent(display, XRootWindow(display, screen), False, SubstructureRedirectMask, &e);


	XGCValues values = { .graphics_exposures = False };
	unsigned long valuemask = 0;
	GC gc = XCreateGC(display, window, valuemask, &values);
	Colormap colormap = DefaultColormap(display, screen);

	for (int i = 0; i < n_circles; i += 1) {
		int circle_size = circles[i].size + circles[i].width;
		XAllocNamedColor(display, colormap, circles[i].color, &(circles[i]._color), &(circles[i]._color));
		XSetForeground(display, gc, circles[i]._color.pixel);
		XSetLineAttributes(display, gc, circles[i].width, LineSolid, CapButt, JoinBevel);

		XDrawArc(display, window, gc,
				size / 2 - circle_size / 4, size / 2 - circle_size / 4,
				circle_size / 2, circle_size / 2,
				0, 360 * 64
			);
	}

	WindowInfo *info = malloc(sizeof(WindowInfo));
	info->window = window;
	info->size = size;
	info->gc = gc;

	return info;
}


int main(int argc, char* argv[]) {
	CursorPosition  cursor_pos;
	Circle          *circles;
	int             n_circles = -1;
	Display         *display;
	int             screen = -1;
	WindowInfo      *window_info;

	char *display_name = getenv("DISPLAY");	
	if (!display_name) {
		fprintf(stderr, "%s: cannot connect to X server '%s'\n", argv[0], display_name);
		exit(1);
	}

	display = XOpenDisplay(display_name);
	screen = DefaultScreen(display);


	n_circles = 2;
	circles = malloc(sizeof(Circle) * n_circles);

	for (int i = 0; i < n_circles; i += 1) {
		circles[i].size = 100 + 10 * i;
		circles[i].width = (i + 1) * 2;
		circles[i].color = i == 0 ? "white" : "red";
	}

	cursor_position_poll(&cursor_pos, display, screen);

	window_info = window_init(display, screen, circles, n_circles);
	XRaiseWindow(display, window_info->window);
	XFlush(display);

	int size = window_info->size;

	for (int i=1; i < 1000000; i+=1) { 
		cursor_position_poll(&cursor_pos, display, screen);
		XMoveWindow(display, window_info->window, cursor_pos.root_x - size / 2, cursor_pos.root_y - size / 2);
		XSync(display, False);
		usleep(16000);
	}

	XFreeGC(display, window_info->gc);
	XCloseDisplay(display);
	free(window_info);

	return 0;
}
