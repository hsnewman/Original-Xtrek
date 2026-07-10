static char sccsid[] = "@(#)xtrekb.c	3.1";

#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>		/* for cursor constants */
#include <X11/Xutil.h>
#if !defined(cray)
#include <sys/types.h>			/* for stat() */
#endif
#include <sys/stat.h>			/* for stat() */
#if !defined(cray)
#include <sys/wait.h>
#endif
#include "defs.h"


#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))


extern void	exit();
extern char	*getenv();
extern int	errno;

char	*ProgramName;
char	*host;
char	*bgcolor;
char	*displayname;
char	*geometry;
char	*fontname, *sfontname;
int	cmdlnfont = 0;
int	cmdlnrv = 0;
int	cmdlngeom = 0;
int	cmdlnserv = 0;
int	update = 60;
int	reverse = 0;
int	got_xtrekfont;
int	exposed = 0;
int	width;
int	height;
int	tbool[4] = { -1, -1, -1, -1 };

typedef	unsigned long	Pixel;

XFontStruct	*xfont, *sxfont;
Display		*display;
int		screen;
Pixel		foreground_pixel, background_pixel;
Pixel		fedcolor, romcolor, klicolor, oricolor;
Colormap	default_colormap;
int		depth;
GC		gc, cleargc, sgc;
Window		win;

#define	XDEFAULT	"Xtrekb"

#define	VIEWS	16	/* for rosette() */
#define	ship_width	16
#define	ship_height	16

static void	init();
static void	redraw_xtrek();


static void
Usage ()
{
	static char *help_message[] = {
		"where options include:",
		"    -update seconds                how often to check for players",
		"    -bg color                      background color",
		"    -rv                            reverse video",
		"    -s xtrekhost		    xtrek server to watch",
		"    -geometry geom                 size of icon",
		"    -display host:dpy              X server to display on",
		"    -font fontname                 font to use",
		NULL	};
	char **cpp;

	fprintf (stderr, "usage:  %s [-options ...]\n", ProgramName);
	for (cpp = help_message; *cpp; cpp++) {
		fprintf (stderr, "%s\n", *cpp);
	}
	fprintf (stderr, "\n");
	exit (1);
}


void
main (ac, av)
int	ac;
char	**av;
{
	register char	*s;
	char		c;
	XEvent		data;
	int		setupsleep;

	ProgramName = av[0];
	ac--;
	av++;

	bgcolor = "black";
	displayname = ":0";
	geometry = "";
	host = "xtrek-server";
	fontname = "xtrek";
	sfontname = "6x10";
	cmdlnfont = 0;
	cmdlnrv = 0;
	cmdlngeom = 0;
	if (s = getenv("XTREKSERVER"))
		host = s;
	if (s = getenv("DISPLAY"))
		displayname = s;

	while (ac) {
		if (**av != '-')
			break;
		if (strcmp(*av, "-s") == 0) {
			if (ac < 2) Usage();
			ac--; av++;
			host = *av;
			cmdlnserv = 1;
		} else if (strcmp(*av, "-bg") == 0) {
			if (ac < 2) Usage();
			ac--; av++;
			bgcolor = *av;
		} else if (strcmp(*av, "-display") == 0) {
			if (ac < 2) Usage();
			ac--; av++;
			displayname = *av;
		} else if (strcmp(*av, "-font") == 0) {
			if (ac < 2) Usage();
			ac--; av++;
			sfontname = *av;
			cmdlnfont = 1;
		} else if (strcmp(*av, "-geometry") == 0) {
			if (ac < 2) Usage();
			ac--; av++;
			geometry = *av;
			cmdlngeom = 1;
		} else if (strcmp(*av, "-rv") == 0) {
			reverse = !reverse;
			cmdlnrv = 1;
		} else if (strcmp(*av, "-update") == 0) {
			if (ac < 2) Usage();
			ac--; av++;
			update = atoi(*av);
			if (update <= 1)
				update = 1;
		} else
			Usage();
		ac--; av++;
	}

	close(0);
	close(1);

	exposed = 0;
	init();

	setupsleep = 1;
	while (1) {
		if (XPending(display) || !exposed) {
			XNextEvent(display, &data);
			switch (data.type) {
				case Expose:
					exposed = 1;
					setupsleep = update;
					tbool[0] = -1;
					tbool[1] = -1;
					tbool[2] = -1;
					tbool[3] = -1;
					break;
				case NoExpose:
					exposed = 0;
					break;
			}
		}
		/* Don't draw until we are exposed. */
		if (exposed)
			redraw_xtrek();
		/* NOTE: This would work much better with an interval timer. */
		sleep(setupsleep);
	}
}

/*
 * private procedures
 */

static void
init ()
{
	XSizeHints	wininfo;
	XGCValues	xgcv;
	long		valuemask;
	register char	*s;
	int		X, Y;
	XColor		def;
	int		uspec;
	XFontStruct	*XLoadQueryFont();

	if ((display = XOpenDisplay(displayname)) == NULL) {
		perror(displayname);
		exit(1);
	}
	screen = XDefaultScreen(display);
	depth = DefaultDepth(display, screen);

	/* Lookup black & white pixels. */
	foreground_pixel = XWhitePixel(display, screen);
	background_pixel = XBlackPixel(display, screen);

	if (!cmdlnserv && (s = XGetDefault(display, XDEFAULT, "server")) == NULL) {
		host = s;
	}
	if ((s = XGetDefault(display, XDEFAULT, "reverseVideo")) != NULL) {
		if (strcmp(s, "on") == 0 || strcmp(s, "true") == 0 || strcmp(s, "1") == 0)
			reverse = 1;
		else if (strcmp(s, "off") == 0 || strcmp(s, "false") == 0 || strcmp(s, "0") == 0)
			reverse = 0;
	}
	if (cmdlnrv)
		reverse = !reverse;

	if (reverse) {
		Pixel tmp;

		tmp = foreground_pixel;
		foreground_pixel = background_pixel;
		background_pixel = tmp;
	}

	if (!cmdlnfont && (s = XGetDefault(display, XDEFAULT, "font")) != NULL)
		sfontname = s;

	sxfont = XLoadQueryFont(display, sfontname);

	xfont = XLoadQueryFont(display, fontname);
	if (xfont == (XFontStruct *) NULL) {
		fprintf(stderr, "Xtrek font not available.\n");
		fontname = "6x10";
		xfont = XLoadQueryFont(display, fontname);
		if (xfont == (XFontStruct *) NULL) {
			fprintf(stderr, "default font (6x10) not available.\n");
			XCloseDisplay(display);
			exit(1);
			/*NOTREACHED*/
		}
	}
	got_xtrekfont = (strcmp(fontname, "xtrek") == 0);

	if (!cmdlngeom && (s = XGetDefault(display, XDEFAULT, "geometry")) != NULL) {
		geometry = s;
		cmdlngeom = 1;	/* Not really, but do uspec stuff below. */
	}
	width = ship_width * 2;
	height = ship_height * 2;
	X = Y = 0;
	if (cmdlngeom) {
		uspec = XParseGeometry(geometry, &X, &Y, &width, &height);
		if ((uspec & (XValue|XNegative)) == (XValue|XNegative))
			X = -X;
		if ((uspec & (YValue|YNegative)) == (YValue|YNegative))
			Y = -Y;
		if ((uspec & WidthValue) == 0)
			width = ship_width * 2;
		if ((uspec & HeightValue) == 0)
			height = ship_height * 2;
	}
	if (sxfont) {
		if (XTextWidth(sxfont, host, strlen(host)) > width)
			width = XTextWidth(sxfont, host, strlen(host));
		height += (4 + (sxfont->ascent + sxfont->descent));
	}

	win = XCreateWindow(display, RootWindow(display, screen), X, Y,
		width, height, 1, depth, InputOutput,
		(Visual *)CopyFromParent, 0L, (XSetWindowAttributes *) NULL);
	wininfo.x = X;
	wininfo.y = Y;
	wininfo.width = width;
	wininfo.height = height;
	wininfo.min_width = ship_width * 2;
	wininfo.min_height = ship_height * 2;
	wininfo.max_width = ship_width * 2;
	wininfo.max_height = ship_height * 2;
	wininfo.flags = PMinSize | PMaxSize;
	if (uspec & (XValue|YValue))
		wininfo.flags |= USPosition;
	else
		wininfo.flags |= PPosition;
	if (uspec & (WidthValue|HeightValue))
		wininfo.flags |= USSize;
	else
		wininfo.flags |= PSize;

	XSetWindowBackground(display, win, background_pixel);
	XSetStandardProperties(display, win, XDEFAULT, XDEFAULT, NULL, NULL, 0, &wininfo);
	XSelectInput(display, win, ExposureMask);

	valuemask = GCForeground | GCBackground | GCFunction | GCFont;
	xgcv.foreground = foreground_pixel;
	xgcv.background = background_pixel;
	xgcv.function = GXcopy;
	xgcv.font = xfont->fid;
	gc = XCreateGC(display, win, valuemask, &xgcv);
	if (sxfont) {
		xgcv.font = sxfont->fid;
		sgc = XCreateGC(display, win, valuemask, &xgcv);
	}
	xgcv.foreground = background_pixel;
	xgcv.function = GXcopy;
	cleargc = XCreateGC(display, win, GCForeground|GCFunction, &xgcv);

	/* Setup colors */
	if (depth <= 2) {
		fedcolor = romcolor = klicolor = oricolor = foreground_pixel;
	} else {
		default_colormap = XDefaultColormap(display, screen);
		def.pixel = 0;
		XParseColor(display, default_colormap, "yellow", &def);
		XAllocColor(display, default_colormap, &def);
		fedcolor = def.pixel;
		XParseColor(display, default_colormap, "red", &def);
		XAllocColor(display, default_colormap, &def);
		romcolor = def.pixel;
		XParseColor(display, default_colormap, "green", &def);
		XAllocColor(display, default_colormap, &def);
		klicolor = def.pixel;
		XParseColor(display, default_colormap, "#0ff", &def);
		XAllocColor(display, default_colormap, &def);
		oricolor = def.pixel;
	}

	XMapWindow(display, win);
	/* Reset height */
	height = ship_height * 2;
	return;
}

/*
 * drawing code
 */

static void
redraw_xtrek()
{
	register int	x, y;
	int		child, pid;
#if !defined(cray)
	union wait	status;
#else
	int		status;
#endif
	int		ntbool[4];
	char		glyph;
	int		hx, hy;
	int		dohost;
	static XRectangle	kliarea, oriarea, romarea, fedarea;
	static int	areainit = 0;

	child = fork();
	if (child < 0)
		return;

	if (child == 0) {
		execlp("xtrek", "xtrek", "-q", "-s", host, 0);
		exit(-1);
	} else {
		/* Wait for child to exit. */
		do {
			pid = wait(&status);
		} while (pid >= 0 && pid != child);
	}

#if !defined(cray)
	if (status.w_retcode == 0xFF) {
		exit(-1);
	}

	ntbool[0] = status.w_retcode & (1<<3);
	ntbool[1] = status.w_retcode & (1<<2);
	ntbool[2] = status.w_retcode & (1<<1);
	ntbool[3] = status.w_retcode & (1<<0);
#else
	if ((status & 0xFF) != 0) {
		exit(-1);
	}

	status >>= 8;
	status &= 0xFF;
	ntbool[0] = status & (1<<3);
	ntbool[1] = status & (1<<2);
	ntbool[2] = status & (1<<1);
	ntbool[3] = status & (1<<0);
#endif

	dohost = 0;
	hx = x = (width / 2);
	hy = y = (height / 2);
	x = (x - ship_width) / 2;
	y = (y - ship_height) / 2;
	if (sxfont) {
		y += (2 + (sxfont->ascent + sxfont->descent));
	}

	if (!areainit) {
		areainit++;
		kliarea.x = x; kliarea.y = y;
		kliarea.width = ship_width; kliarea.height = ship_height;
		oriarea.x = x + hx; oriarea.y = y;
		oriarea.width = ship_width; oriarea.height = ship_height;
		romarea.x = x; romarea.y = y + hy;
		romarea.width = ship_width; romarea.height = ship_height;
		fedarea.x = x + hx; fedarea.y = y + hy;
		fedarea.width = ship_width; fedarea.height = ship_height;
	}

	if (ntbool[2] != tbool[2]) {
		XFillRectangles(display, win, cleargc, &kliarea, 1);
		XSetForeground(display, gc, klicolor);
		if (got_xtrekfont)
			glyph = KLI_GLYPHS + rosette(ntbool[2] ? 0 : 128);
		else
			glyph = ntbool[2] + '0';
		XDrawString(display, win, gc, x + 0, y + 0, &glyph, 1);
		dohost++;
	}
	tbool[2] = ntbool[2];

	if (ntbool[3] != tbool[3]) {
		XFillRectangles(display, win, cleargc, &oriarea, 1);
		XSetForeground(display, gc, oricolor);
		if (got_xtrekfont)
			glyph = ORI_GLYPHS + rosette(ntbool[3] ? 0 : 128);
		else
			glyph = ntbool[3] + '0';
		XDrawString(display, win, gc, x + hx, y + 0, &glyph, 1);
		dohost++;
	}
	tbool[3] = ntbool[3];

	if (ntbool[1] != tbool[1]) {
		XFillRectangles(display, win, cleargc, &romarea, 1);
		XSetForeground(display, gc, romcolor);
		if (got_xtrekfont)
			glyph = ROM_GLYPHS + rosette(ntbool[1] ? 0 : 128);
		else
			glyph = ntbool[1] + '0';
		XDrawString(display, win, gc, x + 0, y + hy, &glyph, 1);
		dohost++;
	}
	tbool[1] = ntbool[1];

	if (ntbool[0] != tbool[0]) {
		XFillRectangles(display, win, cleargc, &fedarea, 1);
		XSetForeground(display, gc, fedcolor);
		if (got_xtrekfont)
			glyph = FED_GLYPHS + rosette(ntbool[0] ? 0 : 128);
		else
			glyph = ntbool[0] + '0';
		XDrawString(display, win, gc, x + hx, y + hy, &glyph, 1);
		dohost++;
	}
	tbool[0] = ntbool[0];

	if (dohost && sxfont) {
		XDrawImageString(display, win, sgc, 1, 1 + sxfont->ascent, host, strlen(host));
	}
	XFlush(display);
	return;
}
