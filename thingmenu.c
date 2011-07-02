/*
 * Copy me if you can.
 * by 20h
 */
#include <unistd.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/extensions/XTest.h>

/* macros */
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#define LENGTH(x)       (sizeof x / sizeof x[0])

/* enums */
enum { ColFG, ColBG, ColLast };
enum { NetWMWindowType, NetLast };

/* typedefs */
typedef unsigned int uint;
typedef unsigned long ulong;

typedef struct {
	ulong norm[ColLast];
	ulong press[ColLast];
	ulong high[ColLast];

	Drawable drawable;
	GC gc;
	struct {
		int ascent;
		int descent;
		int height;
		XFontSet set;
		XFontStruct *xfont;
	} font;
} DC; /* draw context */

typedef struct {
	char *label;
	char *cmd;
	uint width;
	int x, y, w, h;
	Bool highlighted;
	Bool pressed;
	Bool forceexit;
} Entry;

/* function declarations */
static void motionnotify(XEvent *e);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void cleanup(void);
static void configurenotify(XEvent *e);
static void unmapnotify(XEvent *e);
static void die(const char *errstr, ...);
static void drawmenu(void);
static void drawentry(Entry *e);
static void expose(XEvent *e);
static Entry *findentry(int x, int y);
static ulong getcolor(const char *colstr);
static void initfont(const char *fontstr);
static void leavenotify(XEvent *e);
static void press(Entry *e);
static void run(void);
static void setup(void);
static int textnw(const char *text, uint len);
static void unpress(void);
static void updateentries(void);

/* variables */
static int screen;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ButtonRelease] = buttonrelease,
	[ConfigureNotify] = configurenotify,
	[UnmapNotify] = unmapnotify,
	[Expose] = expose,
	[LeaveNotify] = leavenotify,
	[MotionNotify] = motionnotify
};

static Display *dpy;
static DC dc;
static Window root, win;
static Bool running = True, horizontal = False;
/*
 * ww = window width; www = wanted width window; wh = window height;
 * wx = window x position; wy = window y position;
 */
static int ww = 0, www = 0, wh = 0, wx = 0, wy = 0;
static char *name = "thingmenu";

Entry **entries = NULL;
int nentries = 0;
int oneshot = 1;

/* configuration, allows nested code to access above variables */
#include "config.h"

void
motionnotify(XEvent *e)
{
	XPointerMovedEvent *ev = &e->xmotion;
	int i;

	for(i = 0; i < nentries; i++) {
		if(ev->x > entries[i]->x
				&& ev->x < entries[i]->x + entries[i]->w
				&& ev->y > entries[i]->y
				&& ev->y < entries[i]->y + entries[i]->h) {
			if (entries[i]->highlighted != True) {
				entries[i]->highlighted = True;
				drawentry(entries[i]);
			}
			continue;
		}
		if (entries[i]->highlighted == True) {
			entries[i]->highlighted = False;
			drawentry(entries[i]);
		}
	}
}

void
buttonpress(XEvent *e)
{
	XButtonPressedEvent *ev = &e->xbutton;
	Entry *en;

	if((en = findentry(ev->x, ev->y)))
		press(en);
}

void
buttonrelease(XEvent *e)
{
	XButtonPressedEvent *ev = &e->xbutton;
	Entry *en;

	if((en = findentry(ev->x, ev->y)))
		unpress();
}

void
cleanup(void)
{
	if(dc.font.set)
		XFreeFontSet(dpy, dc.font.set);
	else
		XFreeFont(dpy, dc.font.xfont);
	XFreePixmap(dpy, dc.drawable);
	XFreeGC(dpy, dc.gc);
	XDestroyWindow(dpy, win);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

void
configurenotify(XEvent *e)
{
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == win && (ev->width != ww || ev->height != wh)) {
		ww = ev->width;
		wh = ev->height;
		XFreePixmap(dpy, dc.drawable);
		dc.drawable = XCreatePixmap(dpy, root, ww, wh,
				DefaultDepth(dpy, screen));
		updateentries();
	}
}

void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
drawmenu(void)
{
	int i;

	for(i = 0; i < nentries; i++)
		drawentry(entries[i]);
	XSync(dpy, False);
}

void
drawentry(Entry *e)
{
	int x, y, h, len;
	XRectangle r = { e->x, e->y, e->w, e->h};
	const char *l;
	ulong *col;

	if(e->pressed)
		col = dc.press;
	else if(e->highlighted)
		col = dc.high;
	else
		col = dc.norm;

	XSetForeground(dpy, dc.gc, col[ColBG]);
	XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	XSetForeground(dpy, dc.gc, dc.norm[ColFG]);
	r.height -= 1;
	r.width -= 1;
	XDrawRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	XSetForeground(dpy, dc.gc, col[ColFG]);

	l = e->label;
	len = strlen(l);
	h = dc.font.height;
	y = e->y + (e->h / 2) - (h / 2) + dc.font.ascent;
	x = e->x + (e->w / 2) - (textnw(l, len) / 2);
	if(dc.font.set) {
		XmbDrawString(dpy, dc.drawable, dc.font.set, dc.gc, x, y, l,
				len);
	} else
		XDrawString(dpy, dc.drawable, dc.gc, x, y, l, len);
	XCopyArea(dpy, dc.drawable, win, dc.gc, e->x, e->y, e->w, e->h,
			e->x, e->y);
}

void
unmapnotify(XEvent *e)
{
	running = False;
}

void
expose(XEvent *e)
{
	XExposeEvent *ev = &e->xexpose;

	if(ev->count == 0 && (ev->window == win))
		drawmenu();
}

Entry *
findentry(int x, int y)
{
	int i;

	for(i = 0; i < nentries; i++) {
		if(x > entries[i]->x && x < entries[i]->x + entries[i]->w
				&& y > entries[i]->y
				&& y < entries[i]->y + entries[i]->h) {
			return entries[i];
		}
	}
	return NULL;
}

ulong
getcolor(const char *colstr)
{
	Colormap cmap = DefaultColormap(dpy, screen);
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
		die("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

void
initfont(const char *fontstr)
{
	char *def, **missing;
	int i, n;

	missing = NULL;
	if(dc.font.set)
		XFreeFontSet(dpy, dc.font.set);
	dc.font.set = XCreateFontSet(dpy, fontstr, &missing, &n, &def);
	if(missing) {
		while(n--)
			fprintf(stderr, "svkbd: missing fontset: %s\n", missing[n]);
		XFreeStringList(missing);
	}
	if(dc.font.set) {
		XFontStruct **xfonts;
		char **font_names;
		dc.font.ascent = dc.font.descent = 0;
		n = XFontsOfFontSet(dc.font.set, &xfonts, &font_names);
		for(i = 0, dc.font.ascent = 0, dc.font.descent = 0; i < n; i++) {
			dc.font.ascent = MAX(dc.font.ascent, (*xfonts)->ascent);
			dc.font.descent = MAX(dc.font.descent,(*xfonts)->descent);
			xfonts++;
		}
	}
	else {
		if(dc.font.xfont)
			XFreeFont(dpy, dc.font.xfont);
		dc.font.xfont = NULL;
		if(!(dc.font.xfont = XLoadQueryFont(dpy, fontstr))
		&& !(dc.font.xfont = XLoadQueryFont(dpy, "fixed")))
			die("error, cannot load font: '%s'\n", fontstr);
		dc.font.ascent = dc.font.xfont->ascent;
		dc.font.descent = dc.font.xfont->descent;
	}
	dc.font.height = dc.font.ascent + dc.font.descent;
}

void
leavenotify(XEvent *e)
{
	unpress();
}

void
runentry(Entry *e)
{
	char *shell;

	if (fork()) {
		if (oneshot || e->forceexit) {
			XDestroyWindow(dpy, win);
			exit(0);
		}
		return;
	}
	if (fork())
		exit(0);

	shell = getenv("SHELL");
	if (!shell)
		shell = "/bin/sh";

	execlp(shell, basename(shell), "-c", e->cmd, (char *)NULL);
}

void
press(Entry *e)
{
	e->pressed = !e->pressed;

	runentry(e);
	drawentry(e);
}

void
run(void)
{
	XEvent ev;

	/* main event loop */
	XSync(dpy, False);
	while(running) {
		XNextEvent(dpy, &ev);
		if(handler[ev.type])
			(handler[ev.type])(&ev); /* call handler */
	}
}

void
setup(void)
{
	XSetWindowAttributes wa;
	XTextProperty str;
	XSizeHints *sizeh;
	XClassHint *ch;
	int i, sh, sw, ls;

	/* init screen */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	sw = DisplayWidth(dpy, screen) - 1;
	sh = DisplayHeight(dpy, screen) - 1;
	initfont(font);

	/* init atoms */

	/* init appearance */

	for (i = 0, www = 0; i < nentries; i++) {
		ls = textnw(entries[i]->label,
				strlen(entries[i]->label));
		if (ls > www)
			www = ls;
	}
	www *= 1.5;

	if (ww == 0) {
		if (horizontal) {
			ww = www * nentries;
		} else {
			ww = www;
		}
	}
	if (wh == 0) {
		if (horizontal) {
			wh = dc.font.height * 2;
		} else {
			wh = nentries * (dc.font.height * 2);
		}
	}
	if (wy == 0)
		wy = (sh - wh) / 2;
	if (wx == 0)
		wx = (sw - ww) / 2;

	dc.norm[ColBG] = getcolor(normbgcolor);
	dc.norm[ColFG] = getcolor(normfgcolor);
	dc.press[ColBG] = getcolor(pressbgcolor);
	dc.press[ColFG] = getcolor(pressfgcolor);
	dc.high[ColBG] = getcolor(highlightbgcolor);
	dc.high[ColFG] = getcolor(highlightfgcolor);

	dc.drawable = XCreatePixmap(dpy, root, ww, wh, DefaultDepth(dpy, screen));
	dc.gc = XCreateGC(dpy, root, 0, 0);
	if(!dc.font.set)
		XSetFont(dpy, dc.gc, dc.font.xfont->fid);
	for(i = 0; i < nentries; i++)
		entries[i]->pressed = 0;

	wa.override_redirect = !wmborder;
	wa.border_pixel = dc.norm[ColFG];
	wa.background_pixel = dc.norm[ColBG];
	win = XCreateWindow(dpy, root, wx, wy, ww, wh, 0,
			    CopyFromParent, CopyFromParent, CopyFromParent,
			    CWOverrideRedirect | CWBorderPixel | CWBackingPixel, &wa);
	XSelectInput(dpy, win, StructureNotifyMask|ButtonReleaseMask|
			ButtonPressMask|ExposureMask|LeaveWindowMask|
			PointerMotionMask);

	sizeh = XAllocSizeHints();
	sizeh->flags = PMaxSize | PMinSize;
	sizeh->min_width = sizeh->max_width = ww;
	sizeh->min_height = sizeh->max_height = wh;
	XStringListToTextProperty(&name, 1, &str);
	ch = XAllocClassHint();
	ch->res_class = name;
	ch->res_name = name;

	XSetWMProperties(dpy, win, &str, &str, NULL, 0, sizeh, NULL,
			ch);

	XFree(ch);
	XFree(str.value);
	XFree(sizeh);

	XMapRaised(dpy, win);
	updateentries();
	drawmenu();
}

int
textnw(const char *text, uint len)
{
	XRectangle r;

	if(dc.font.set) {
		XmbTextExtents(dc.font.set, text, len, NULL, &r);
		return r.width;
	}
	return XTextWidth(dc.font.xfont, text, len);
}

void
unpress()
{
	int i;

	for(i = 0; i < nentries; i++) {
		if(entries[i]->pressed) {
			entries[i]->pressed = 0;
			drawentry(entries[i]);
		}
	}
}

void
updateentries(void)
{
	int i, x, y, h, w;

	x = 0;
	y = 0;

	if (horizontal) {
		h = wh;
		w = www;
	} else {
		h = wh / nentries;
		w = ww;
	}
	for(i = 0; i < nentries; i++) {
		entries[i]->x = x;
		entries[i]->y = y;
		entries[i]->w = w;
		entries[i]->h = h;
		if (horizontal) {
			x += w;
		} else {
			y += h;
		}
	}
}

void
usage(char *argv0)
{
	fprintf(stderr, "usage: %s [-hxso] [-wh height] [-ww width] "
			"[-wx x position] [-wy y position] [--] "
			"label:cmd ...\n", argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	Bool addexit;
	char *label, *cmd;
	int i;


	addexit = True;

	if (argc < 2)
		usage(argv[0]);
	i = 1;
	for (; argv[i]; i++) {
		if (argv[i][0] != '-')
			break;

		if (argv[i][1] == '-') {
			i++;
			break;
		}

		switch (argv[i][1]) {
		case 'h':
			usage(argv[0]);
		case 'o':
			horizontal = True;
			break;
		case 's':
			oneshot = 0;
			break;
		case 'w':
			switch ((i >= argc - 1)? 0 : argv[i][2]) {
			case 'h':
				wh = atoi(argv[i+1]);
				i++;
				break;
			case 'w':
				ww = atoi(argv[i+1]);
				i++;
				break;
			case 'x':
				wx = atoi(argv[i+1]);
				i++;
				break;
			case 'y':
				wy = atoi(argv[i+1]);
				i++;
				break;
			default:
				usage(argv[0]);
			}
			break;
		case 'x':
			addexit = False;
			break;
		default:
			usage(argv[0]);
		}
	}

	for (; argv[i]; i++) {
		label = argv[i];
		cmd = strchr(label, ':');
		if (cmd == NULL) {
			cmd = label;
		} else {
			*cmd++ = '\0';
		}

		entries = realloc(entries, sizeof(entries[0])*(++nentries));
		entries[nentries-1] = malloc(sizeof(*entries[0]));
		bzero(entries[nentries-1], sizeof(*entries[0]));

		entries[nentries-1]->label = strdup(label);
		if (entries[nentries-1]->label == NULL)
			die("strdup returned NULL\n");
		entries[nentries-1]->cmd = strdup(cmd);
		if (entries[nentries-1]->cmd == NULL)
			die("strdup returned NULL\n");
	}
	if (nentries < 1)
		usage(argv[0]);

	if (addexit) {
		entries = realloc(entries, sizeof(entries[0])*(++nentries));
		entries[nentries-1] = malloc(sizeof(*entries[0]));
		bzero(entries[nentries-1], sizeof(*entries[0]));
		entries[nentries-1]->label = strdup("cancel");
		entries[nentries-1]->cmd = "exit";
		entries[nentries-1]->forceexit = True;
	}

	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fprintf(stderr, "warning: no locale support\n");
	if(!(dpy = XOpenDisplay(0)))
		die("thingmenu: cannot open display\n");

	setup();
	run();
	cleanup();
	XCloseDisplay(dpy);

	for (i = 0; i < nentries; i++)
		free(entries[i]);
	free(entries);

	return 0;
}

