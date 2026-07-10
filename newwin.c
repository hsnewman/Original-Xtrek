static char sccsid[] = "@(#)newwin.c	3.1";
/*

	Copyright (c) 1986 	Chris Guthrie

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation.  No representations are made about the
suitability of this software for any purpose.  It is
provided "as is" without express or implied warranty.

*/

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#if !defined(cray)
#include <sys/types.h>
#endif

#ifdef hpux
#include <time.h>
#else /* hpux */
#include <sys/time.h>
#endif /* hpux */

#include "defs.h"
#include "data.h"
#include "bitmaps.h"
#include "clock.bitmap"

static char	rvbuf[80];
static int xboxsize, yboxsize;
extern int	debug;

#define SIZEOF(a)	(sizeof (a) / sizeof (*(a)))

#define TILESIDE	32

static char	solid[TILESIDE] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#define	gray_width	16
#define	gray_height	16
/* This can change depending on YAlertPattern */
static char	gray[TILESIDE] = {
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
};
#define	grey_width	16
#define	grey_height	16
/* This stays the same */
static char	grey[TILESIDE] = {
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
	0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
};
static char	striped[TILESIDE] = {
	0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
	0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f,
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
	0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
};


char *
newwin(p)
register struct player	*p;
{
    register int	i;
    register char	*str;
    register struct player	*j;
    unsigned long	mask;
    XSizeHints		*wininfo;
    int			uspec, rootX, rootY, dummy1, dummy2;
    XWMHints		*wmhints;
    XClassHint		*classhint;
    XIconSize		*size_list_return;
    int			count_return;
    int			open_retries;

#ifdef EXCLUDE
    for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	if (j->p_status != PFREE && (j != p) && (j->p_flags & PFROBOT) == 0 &&
	    strcmp(j->p_monitor, p->p_monitor) == 0) {
	    if (j->p_status == PALIVE)
		    sprintf(rvbuf, "%s already playing on %s", j->p_login, p->p_monitor);
	    else
		    sprintf(rvbuf, "Someone already playing on %s", p->p_monitor);
	    return rvbuf;
	}
    }
#endif /* EXCLUDE */
    open_retries = 5;
    while (1) {
	if ((p->display = XOpenDisplay(p->p_monitor)) == NULL) {
	    /* This seems to be fairly common for some reason. */
	    if (open_retries-- > 0 && errno == EINTR)
		continue;
	    perror(p->p_monitor);
	    p->p_status = PFREE;
	    p->p_warncount = 0;
	    sprintf(rvbuf, "Cannot open display %s", p->p_monitor);
	    return rvbuf;
	} else
	    break;
    }

    p->screen = DefaultScreen(p->display);
    p->mono = XDisplayCells(p->display, p->screen) <= 2;
    p->xcn = XConnectionNumber(p->display);
    p->p_border = DEF_BORDER;
    p->p_messagesize = DEF_MESSAGESIZE;
    p->p_xwinsize = DEF_XWINSIZE;
    p->p_ywinsize = DEF_YWINSIZE;

    getColorDefs(p, PROGRAM_NAME);
    rootX = 0;
    rootY = 0;
    uspec = 0;
    if ((str = XGetDefault(p->display, PROGRAM_NAME, "geometry")) != NULL) {
	uspec = XParseGeometry(str, &rootX, &rootY, &dummy1, &dummy2);
	if ((uspec & (XValue|XNegative)) == (XValue|XNegative))
		rootX = -rootX;
	if ((uspec & (YValue|YNegative)) == (YValue|YNegative))
		rootY = -rootY;
    }

    p->baseWin = XCreateWindow(p->display, RootWindow(p->display, p->screen), rootX, rootY,
	(unsigned) p->p_xwinsize * 2 + 1 * p->p_border, (unsigned) p->p_ywinsize + 2 * p->p_border + 2 * p->p_messagesize,
	p->p_border, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    p->wm_change_state = XInternAtom(p->display, "WM_CHANGE_STATE", False);
    p->wm_delete_window = XInternAtom(p->display, "WM_DELETE_WINDOW", False);
    (void) XSetWMProtocols(p->display, p->baseWin, &p->wm_delete_window, 1);

    wininfo = XAllocSizeHints();
    wininfo->x = rootX;
    wininfo->y = rootY;
    wininfo->width = p->p_xwinsize * 2 + 1 * p->p_border;
    wininfo->height = p->p_ywinsize + 2 * p->p_border + 2 * p->p_messagesize;
    wininfo->min_width = clock_width * 5 + 1 * p->p_border;
    wininfo->min_height = clock_height;
    wininfo->min_height += 16 + 2 * p->p_border + 2 * p->p_messagesize;
    wininfo->max_width = DisplayWidth(p->display, p->screen);
    wininfo->max_height = DisplayHeight(p->display, p->screen);
    if (uspec & (XValue|YValue|XNegative|YNegative))
	    wininfo->flags = USPosition | PSize | PMinSize | PMaxSize;
    else
	    wininfo->flags = PPosition | PSize | PMinSize | PMaxSize;
    XSetWMNormalHints(p->display, p->baseWin, wininfo);
    XFree(wininfo);

    XSelectInput(p->display, p->baseWin, ExposureMask|StructureNotifyMask|VisibilityChangeMask);
    XSetWindowBackground(p->display, p->baseWin, p->backColor);
    p->ibm = XCreateBitmapFromData(p->display, p->baseWin, icon_bits, icon_width, icon_height);
    XStoreName(p->display, p->baseWin, PROGRAM_NAME);
    XSetIconName(p->display, p->baseWin, PROGRAM_NAME);

    rootX = 0;
    rootY = 0;
    uspec = 0;
    if ((str = XGetDefault(p->display, PROGRAM_NAME, "icon.geometry")) != NULL) {
	uspec = XParseGeometry(str, &rootX, &rootY, &dummy1, &dummy2);
	if ((uspec & (XValue|XNegative)) == (XValue|XNegative))
		rootX = -rootX;
	if ((uspec & (YValue|YNegative)) == (YValue|YNegative))
		rootY = -rootY;
    }
    p->iconWin = XCreateWindow(p->display, RootWindow(p->display, p->screen), rootX, rootY, (unsigned) icon_width,
	(unsigned) icon_height, p->p_border, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->iconWin, p->backColor);
    XSelectInput(p->display, p->iconWin, ExposureMask|StructureNotifyMask);

    if (debug > 0) {
	count_return = 0;
	i = XGetIconSizes(p->display, RootWindow(p->display, p->screen),
		&size_list_return, &count_return);
	fprintf(stderr, "Result of XGetIconSizes = %d\n", i);
	fprintf(stderr, "We have %d icon sizes, they are:\n", count_return);
	for (i = 0; i < count_return; i++) {
		fprintf(stderr, "\t%d>  %d X %d -> %d X %d (inc: %d X %d)\n", i,
			size_list_return[i].min_width, size_list_return[i].min_height,
			size_list_return[i].max_width, size_list_return[i].max_height,
			size_list_return[i].width_inc, size_list_return[i].height_inc);
	}
    }

    wmhints = XAllocWMHints();
    /* NOTE: Should check for NULL return here. */
    wmhints->icon_window = p->iconWin;
    wmhints->icon_pixmap = p->ibm;
    wmhints->input = 1;
    wmhints->initial_state = NormalState;
    wmhints->flags = IconPixmapHint|IconWindowHint|InputHint|StateHint;
    XSetWMHints(p->display, p->baseWin, wmhints);
    XFree(wmhints);

    classhint = XAllocClassHint();
    /* NOTE: Should check for NULL return here. */
    classhint->res_name = PROGRAM_NAME;
    classhint->res_class = PROGRAM_NAME;
    XSetClassHint(p->display, p->baseWin, classhint);
    XFree(classhint);

    p->p_status = PMAP;
    return (char *) NULL;
}

char *
neww2(p)
register struct player	*p;
{
    register int	i;
    register char	*str;
    register struct player	*j;
    unsigned long	mask;
    XGCValues		values;
    XSizeHints		*wininfo;
    int			uspec, rootX, rootY, dummy1, dummy2;
    int			x1, y1, w1, h1;

    values.graphics_exposures = 0;
    values.foreground = p->textColor;
    values.background = p->backColor;
    p->bmgc = XCreateGC(p->display, p->baseWin, GCForeground|GCBackground|GCGraphicsExposures, &values);
    p->gc = XCreateGC(p->display, p->baseWin, (unsigned long) 0, NULL);
    values.function = GXcopy;
    values.foreground = p->backColor;
    p->cleargc = XCreateGC(p->display, p->baseWin, GCFunction|GCForeground, &values);

    p->w = XCreateWindow(p->display, p->baseWin, -p->p_border, -p->p_border,
	(unsigned) (w1 = p->p_xwinsize), (unsigned) p->p_ywinsize,
	p->p_border, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->w, p->backColor);
    XSetWindowBorder(p->display, p->w, p->borderColor);

    if (getFonts(p, PROGRAM_NAME)) {
	XCloseDisplay(p->display);
	sprintf(rvbuf, "Not all fonts are available on %s\n", p->p_monitor);
	return rvbuf;
    }

    p->mapw = XCreateWindow(p->display, p->baseWin, p->p_xwinsize, -p->p_border,
	(unsigned) p->p_xwinsize, (unsigned) p->p_ywinsize, p->p_border,
	DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->mapw, p->backColor);
    XSetWindowBorder(p->display, p->mapw, p->borderColor);

    p->tstatw = XCreateWindow(p->display, p->baseWin, (x1 = -p->p_border), (y1 = p->p_xwinsize),
	w1, (unsigned) (h1 = (p->p_messagesize * 2 + p->p_border)), p->p_border,
	DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->tstatw, p->backColor);
    XSetWindowBorder(p->display, p->tstatw, p->borderColor);

    p->warnw = XCreateWindow(p->display, p->baseWin,
	(x1 = ((p->p_flags&PFNOMAPMODE) ? x1 : w1)),
	(y1 = ((p->p_flags&PFNOMAPMODE) ? (p->p_ywinsize+h1) : p->p_ywinsize)),
	w1, (unsigned) p->p_messagesize, p->p_border,
	DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->warnw, p->backColor);
    XSetWindowBorder(p->display, p->warnw, p->borderColor);

    p->messagew = XCreateWindow(p->display, p->baseWin,
	x1, y1 + p->p_border + p->p_messagesize,
	w1, (unsigned) p->p_messagesize, p->p_border,
        DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->messagew, p->backColor);
    XSetWindowBorder(p->display, p->messagew, p->borderColor);

    p->planetw = XCreateWindow(p->display, p->w, 3, 3, (unsigned) 60 * fontWidth(p->dfont),
	(unsigned) (MAXPLANETS + 3) * fontHeight(p->dfont), 2, DefaultDepth(p->display, p->screen), InputOutput,
	(Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->planetw, p->backColor);
    XSetWindowBorder(p->display, p->planetw, p->borderColor);

    p->playerw = XCreateWindow(p->display, p->w, 3, 3, (unsigned) 66 * fontWidth(p->dfont),
	(unsigned) (MAXPLAYER + 3) * fontHeight(p->dfont), 2, DefaultDepth(p->display, p->screen), InputOutput,
	(Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->playerw, p->backColor);
    XSetWindowBorder(p->display, p->playerw, p->borderColor);

    p->helpWin = XCreateWindow(p->display, RootWindow(p->display, p->screen),
	0, p->p_ywinsize + 2 * p->p_border + 2 * p->p_messagesize,
	(unsigned) p->p_xwinsize * 2 + 1 * p->p_border, (unsigned) 10 * fontHeight(p->dfont), p->p_border,
        DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    (void) XSetWMProtocols(p->display, p->helpWin, &p->wm_delete_window, 1);

    wininfo = XAllocSizeHints();
    wininfo->x = 0;
    wininfo->y = p->p_ywinsize + 2 * p->p_border + 2 * p->p_messagesize;
    wininfo->width = p->p_xwinsize * 2 + 1 * p->p_border;
    wininfo->height = 10 * fontHeight(p->dfont);
    wininfo->min_width = p->p_xwinsize * 2 + 1 * p->p_border;
    wininfo->min_height = 10 * fontHeight(p->dfont);
    wininfo->max_width = p->p_xwinsize * 2 + 1 * p->p_border;
    wininfo->max_height = 20 * fontHeight(p->dfont);
    wininfo->flags = PPosition | PSize | PMinSize | PMaxSize;
    XSetWMNormalHints(p->display, p->helpWin, wininfo);
    XFree(wininfo);

    XStoreName(p->display, p->helpWin, "xtrek-help");
    XSetWindowBackground(p->display, p->helpWin, p->backColor);
    XSetWindowBorder(p->display, p->helpWin, p->borderColor);

    XDefineCursor(p->display, p->baseWin, (Cursor) XCreateFontCursor(p->display, XC_crosshair));
    XDefineCursor(p->display, p->iconWin, (Cursor) XCreateFontCursor(p->display, XC_crosshair));

/* These windows will be used for setting one's warlike stats */

#define WARHEIGHT (fontHeight(p->dfont) * 2)
#define WARWIDTH (fontWidth(p->dfont) * 20)
#define WARBORDER (p->p_border / 2)
    p->war = XCreateWindow(p->display, p->baseWin, p->p_xwinsize + 10, -p->p_border + 10, (unsigned) WARWIDTH,
	(unsigned) WARHEIGHT * 6, WARBORDER, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->war, p->backColor);
    XSetWindowBorder(p->display, p->war, p->borderColor);

    p->warf = XCreateWindow(p->display, p->war, 0, 0 * WARHEIGHT, (unsigned) WARWIDTH,
	 (unsigned) WARHEIGHT, WARBORDER, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->warf, p->backColor);
    XSetWindowBorder(p->display, p->warf, p->borderColor);

    p->warr = XCreateWindow(p->display, p->war, 0, 1 * WARHEIGHT, (unsigned) WARWIDTH,
	 (unsigned) WARHEIGHT, WARBORDER, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->warr, p->backColor);
    XSetWindowBorder(p->display, p->warr, p->borderColor);

    p->wark = XCreateWindow(p->display, p->war, 0, 2 * WARHEIGHT, (unsigned) WARWIDTH,
	 (unsigned) WARHEIGHT, WARBORDER, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->wark, p->backColor);
    XSetWindowBorder(p->display, p->wark, p->borderColor);

    p->waro = XCreateWindow(p->display, p->war, 0, 3 * WARHEIGHT, (unsigned) WARWIDTH,
	 (unsigned) WARHEIGHT, WARBORDER, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->waro, p->backColor);
    XSetWindowBorder(p->display, p->waro, p->borderColor);

    p->wargo = XCreateWindow(p->display, p->war, 0, 4 * WARHEIGHT, (unsigned) WARWIDTH,
	 (unsigned) WARHEIGHT, WARBORDER, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->wargo, p->backColor);
    XSetWindowBorder(p->display, p->wargo, p->borderColor);

    p->warno = XCreateWindow(p->display, p->war, 0, 5 * WARHEIGHT, (unsigned) WARWIDTH,
	 (unsigned) WARHEIGHT, WARBORDER, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->warno, p->backColor);
    XSetWindowBorder(p->display, p->warno, p->borderColor);


    XMapWindow(p->display, p->warf);
    XMapWindow(p->display, p->warr);
    XMapWindow(p->display, p->wark);
    XMapWindow(p->display, p->waro);
    XMapWindow(p->display, p->wargo);
    XMapWindow(p->display, p->warno);

    getResources(p, PROGRAM_NAME);
    mask = GCForeground | GCBackground;
    values.foreground = p->borderColor;
    values.background = p->backColor;
    XChangeGC(p->display, p->gc, mask, &values);
    XSetBackground(p->display, p->gc, (unsigned long) p->backColor);
    if (!p->mono) {
	    XSetWindowBorder(p->display, p->baseWin, p->gColor);
/*ICCCM 4.1.9	    XSetWindowBorder(p->display, p->iconWin, p->gColor);*/
    } else {
	    XSetWindowBorderPixmap(p->display, p->baseWin, p->gTile);
/*ICCCM 4.1.9	    XSetWindowBorder(p->display, p->iconWin, p->gTile);*/
    }
    p->p_status = POUTFIT;
    mapAll(p);
    return (char *) NULL;
}

resizewins(p, cnf)
register struct player	*p;
XConfigureEvent		*cnf;
{
	register int	x1, y1, w1, h1;
	XWindowAttributes wa;

	XGetWindowAttributes(p->display, p->baseWin, &wa);

	/* We only handle baseWin resizes. */
	if (cnf->window != p->baseWin)
		return;

	if (cnf->width > (DisplayWidth(p->display, p->screen) - 15)) {
		cnf->width = DisplayWidth(p->display, p->screen) - 15;
	}
	if (cnf->height > (DisplayHeight(p->display, p->screen) - 15)) {
		cnf->height = DisplayHeight(p->display, p->screen) - 15;
	}

	/* NO! (not yet) */
/*	p->p_border = cnf->border_width;*/

	p->p_xwinsize = (cnf->width - p->p_border) / 2;
	p->p_ywinsize = (cnf->height - 2 * (p->p_border + p->p_messagesize));
	p->p_ts_offset = 0;	/* Reset Status Line Offset */

	if (p->p_xwinsize < DEF_XWINSIZE || p->p_ywinsize < DEF_YWINSIZE) {
		p->p_flags |= PFNOMAPMODE;
		p->p_mapmode = 0;
		p->p_ywinsize -= 2 * (p->p_messagesize + p->p_border);
		p->p_xwinsize *= 2;
		if (p->mapw && ismapped(p, p->mapw))
			XUnmapWindow(p->display, p->mapw);
	} else {
		if (p->p_flags & PFNOMAPMODE)
			p->p_mapmode = 1;
		p->p_flags &= ~PFNOMAPMODE;
		if (p->mapw && (!ismapped(p, p->mapw)))
			XMapWindow(p->display, p->mapw);
	}
	y1 = clock_height + 4 * p->p_messagesize + 3 * p->p_border;
	if (p->p_ywinsize < y1)
		p->p_ywinsize = y1;

#ifdef notdef	/* This really screws up something...I suspect some */
		/* violation of ICCCM. */
	XResizeWindow(p->display, p->baseWin,
		(p->p_flags & PFNOMAPMODE) ? (p->p_xwinsize + p->p_border) : (p->p_xwinsize * 2 + p->p_border),
		(p->p_flags & PFNOMAPMODE) ? (p->p_ywinsize + 4  * (p->p_messagesize + p->p_border)) : (p->p_ywinsize + 2 * (p->p_border + p->p_messagesize)));
#endif

	if (p->w)
		XMoveResizeWindow(p->display, p->w, -p->p_border, -p->p_border,
			(w1 = p->p_xwinsize), p->p_ywinsize);

	if (p->mapw)
		XMoveResizeWindow(p->display, p->mapw, p->p_xwinsize, -p->p_border, p->p_xwinsize, p->p_ywinsize);

	if (p->tstatw)
		XMoveResizeWindow(p->display, p->tstatw,
			(x1 = -p->p_border), (y1 = p->p_ywinsize),
			w1, (h1 = (p->p_border + 2 * p->p_messagesize)));

	if (p->warnw)
		XMoveResizeWindow(p->display, p->warnw,
			(x1 = ((p->p_flags & PFNOMAPMODE) ? x1 : w1)),
			(y1 = ((p->p_flags & PFNOMAPMODE) ? (p->p_ywinsize+h1) : p->p_ywinsize)),
			w1, p->p_messagesize);

	if (p->messagew)
		XMoveResizeWindow(p->display, p->messagew,
			x1,
			y1 + p->p_border + p->p_messagesize,
			w1, p->p_messagesize);

	XClearWindow(p->display, p->baseWin);
	p->p_redrawall = 1;
}

char *
winmapped(p)
register struct player	*p;
{
	return (neww2(p));
}

/*
 * this is separate from newwin.  It should; be called *after*
 * openmem.  If not, expose events will be eaten by the forked
 * process (daemon).
 */
mapAll(p)
register struct player *p;
{
    initinput(p);
    XMapWindow(p->display, p->mapw);
    XMapWindow(p->display, p->tstatw);
    XMapWindow(p->display, p->warnw);
    XMapWindow(p->display, p->messagew);
    XMapWindow(p->display, p->w);
}

mapBase(p)
register struct player *p;
{
    XMapWindow(p->display, p->baseWin);
}

/* This routine throws up an entry window for the player. */

entrywindow(p)
register struct player	*p;
{
	int boxy;

	xboxsize = p->p_xwinsize < 500 ? (p->p_xwinsize/5) : 100;
	yboxsize = p->p_ywinsize < 100 ? p->p_ywinsize : 100;
	boxy = p->p_ywinsize - yboxsize;
	if (boxy < 0)
		boxy = 0;

	/* Can we enter at other systems? */
	checksystems(p);

    /* The following allows quick choosing of teams */
    if ((p->p_pick & (1 << FED)) && !p->mustexit) {
	p->fwin = XCreateWindow(p->display, p->w, 0 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize,
	    1, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->fwin, KeyPressMask|ButtonPressMask|ButtonReleaseMask|
		ExposureMask|SubstructureNotifyMask);
	XSetWindowBackground(p->display, p->fwin, p->backColor);
	XMapWindow(p->display, p->fwin);
    }
    else {
	p->fwin = XCreateWindow(p->display, p->w, 0 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize,
	    1, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->fwin, ExposureMask|SubstructureNotifyMask);
	XSetWindowBackgroundPixmap(p->display, p->fwin, p->stippleTile);
	XMapWindow(p->display, p->fwin);
    }
    XSetWindowBorder(p->display, p->fwin, p->shipCol[FED]);

    if ((p->p_pick & (1 << ROM)) && !p->mustexit) {
	p->rwin = XCreateWindow(p->display, p->w, 1 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize, 1,
	    DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->rwin, KeyPressMask|ButtonPressMask|ButtonReleaseMask|
	    ExposureMask|SubstructureNotifyMask);
	XSetWindowBackground(p->display, p->rwin, p->backColor);
	XMapWindow(p->display, p->rwin);
    }
    else {
	p->rwin = XCreateWindow(p->display, p->w, 1 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize,
	    1, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->rwin, ExposureMask|SubstructureNotifyMask);
	XSetWindowBackgroundPixmap(p->display, p->rwin, p->stippleTile);
	XMapWindow(p->display, p->rwin);
    }
    XSetWindowBorder(p->display, p->rwin, p->shipCol[ROM]);

    if ((p->p_pick & (1 << KLI)) && !p->mustexit) {
	p->kwin = XCreateWindow(p->display, p->w, 2 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize,
	    1, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->kwin, KeyPressMask|ButtonPressMask|ButtonReleaseMask|
	    ExposureMask|SubstructureNotifyMask);
	XSetWindowBackground(p->display, p->kwin, p->backColor);
	XMapWindow(p->display, p->kwin);
    }
    else {
	p->kwin = XCreateWindow(p->display, p->w, 2 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize,
	    1, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->kwin, ExposureMask|SubstructureNotifyMask);
	XSetWindowBackgroundPixmap(p->display, p->kwin, p->stippleTile);
	XMapWindow(p->display, p->kwin);
    }
    XSetWindowBorder(p->display, p->kwin, p->shipCol[KLI]);

    if ((p->p_pick & (1 << ORI)) && !p->mustexit) {
	p->owin = XCreateWindow(p->display, p->w, 3 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize,
	    1, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->owin, KeyPressMask|ButtonPressMask|ButtonReleaseMask|
	    ExposureMask|SubstructureNotifyMask);
	XSetWindowBackground(p->display, p->owin, p->backColor);
	XMapWindow(p->display, p->owin);
    }
    else {
	p->owin = XCreateWindow(p->display, p->w, 3 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize,
	    1, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSelectInput(p->display, p->owin, ExposureMask|SubstructureNotifyMask);
	XSetWindowBackgroundPixmap(p->display, p->owin, p->stippleTile);
	XMapWindow(p->display, p->owin);
    }
    XSetWindowBorder(p->display, p->owin, p->shipCol[ORI]);

    p->qwin = XCreateWindow(p->display, p->w, 4 * xboxsize, boxy, (unsigned) xboxsize, (unsigned) yboxsize, 1,
        DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    XSetWindowBackground(p->display, p->qwin, p->backColor);
    XSetWindowBorder(p->display, p->qwin, p->textColor);

    XSelectInput(p->display, p->qwin, KeyPressMask|ButtonPressMask|ExposureMask|SubstructureNotifyMask);
    XMapWindow(p->display, p->qwin);
    XClearWindow(p->display, p->qwin);

    p->startTime = time(0);
    makeClock(p, p->qwin);
}

void
del_entrywindow(p)
register struct player	*p;
{
    destroyClock(p);
    XDestroyWindow(p->display, p->fwin);
    XDestroyWindow(p->display, p->rwin);
    XDestroyWindow(p->display, p->kwin);
    XDestroyWindow(p->display, p->owin);
    XDestroyWindow(p->display, p->qwin);
    p->fwin = p->rwin = p->kwin = p->owin = p->qwin = (Window) NULL;
}

numShips(owner)
{
	int		i, num = 0;
	struct player	*p;

	for (i = 0, p = players; i < MAXPLAYER; i++, p++)
		if (p->p_status == PALIVE && p->p_ship->s_team == owner)
			num++;
	return (num);
}

static char	*AUTHOR[] = {
    "",
    "---  XTREK Release Version 4.0 (D3.1) ---",
    "",
    "By Chris Guthrie (chris at ic.berkeley.edu)",
    "And Ed James (edjames at ic.berkeley.edu)",
    "",
    "Many more changes by Dan A. Dickey (ddickey at cray.com)"
};

showMotd(p)
register struct player	*p;
{
    char buf[BUFSIZ];
    FILE *motd, *fopen();
    int	i, length, top, center;

    /* Author Gratification */
    XClearWindow(p->display, p->w);
    for (i = 0; i < SIZEOF(AUTHOR); i++) {
	length = strlen(AUTHOR[i]);
	center = p->p_xwinsize / 2 - (XTextWidth(p->dfont, AUTHOR[i], length)) / 2;
	XDrawImageString(p->display, p->w, p->dfgc, center, i * fontHeight(p->dfont) + p->dfont->ascent, AUTHOR[i],
	    length);
    }
    top = SIZEOF(AUTHOR) + 2;

    /* the following will print a motd */
    if ((motd = fopen(MOTD, "r")) != NULL) {
	for (i = top; fgets(buf, sizeof (buf), motd) != NULL; i++) {
	    length = strlen(buf);
	    buf[length-1] = NULL;
	    length = XTextWidth(p->dfont, buf, length);
	    if (length > ((p->p_xwinsize/2)-20))
		length = ((p->p_xwinsize/2)-20);
	    XDrawImageString(p->display, p->w, p->dfgc, 20, i * fontHeight(p->dfont) + p->dfont->ascent,
		buf, strlen(buf));
	}
	(void) fclose(motd);
    }
}

getResources(p, prog)
register struct player	*p;
char			*prog;
{
    getTiles(p, prog);
    initStats(p, prog);

    if (booleanDefault(p, prog, "showShields")) {
	    p->p_flags |= PFSHOWSHIELDS;
	    if (debug)
		fprintf(stderr, "pno %d PFSHOWSHIELDS\n", p->p_no);
    }
    if (booleanDefault(p, prog, "showStats")) {
	    p->p_flags |= PFRSHOWSTATS;
	    if (debug)
		fprintf(stderr, "pno %d PFRSHOWSTATS\n", p->p_no);
    }
}

getTiles(p, prog)
register struct player	*p;
char			*prog;
{
	register int	i;
	char	rPatt[TILESIDE], yPatt[TILESIDE], gPatt[TILESIDE];
	char	tbPatt[TILESIDE];
	int	rSize = sizeof (rPatt) / 2;
	int	ySize = sizeof (yPatt) / 2;
	int	gSize = sizeof (gPatt) / 2;
	int	tbSize = sizeof (tbPatt) / 2;
	XImage	*image;
	GC	gc;
	XGCValues	gcv;

	if (p->mono) {
		if (p->backColor == XWhitePixel(p->display, p->screen)) {
			/* Reverse video on, so reverse the bitmaps. */
			for (i = 0; i < TILESIDE; i++) solid[i] = ~solid[i];
			for (i = 0; i < TILESIDE; i++) gray[i] = ~gray[i];
			for (i = 0; i < TILESIDE; i++) striped[i] = ~striped[i];
		}
		if (arrayDefault(p, prog, "RalertPattern", &rSize, rPatt) < 0) {
			rSize = TILESIDE / 2;
			bcopy(striped, rPatt, sizeof (rPatt));
		}
		if (arrayDefault(p, prog, "YalertPattern", &ySize, yPatt) < 0) {
			ySize = TILESIDE / 2;
			bcopy(gray, yPatt, sizeof (yPatt));
		}
		if (arrayDefault(p, prog, "GalertPattern", &gSize, gPatt) < 0) {
			gSize = TILESIDE / 2;
			bcopy(solid, gPatt, sizeof (gPatt));
		}

		p->rTile = XCreateBitmapFromData(p->display, p->w, rPatt, rSize, rSize);
		p->yTile = XCreateBitmapFromData(p->display, p->w, yPatt, ySize, ySize);
		p->gTile = XCreateBitmapFromData(p->display, p->w, gPatt, gSize, gSize);
	}

	gcv.foreground = p->textColor;
	gcv.background = p->backColor;
	gcv.line_style = LineOnOffDash;
	p->tbgc = XCreateGC(p->display, p->baseWin, GCForeground|GCBackground|GCLineStyle, &gcv);

	p->stippleTile = XCreatePixmap(p->display, p->baseWin,
	    stipple_width, stipple_height, DefaultDepth(p->display, p->screen));
	gcv.foreground = p->backColor;
	gcv.background = p->textColor;
	gc = XCreateGC(p->display, p->stippleTile, GCForeground|GCBackground, &gcv);
	image = XCreateImage(p->display, DefaultVisual(p->display, p->screen),
	     1, XYBitmap, 0, stipple_bits, stipple_width, stipple_height, 8, 0);
	XPutImage(p->display, p->stippleTile, gc, image, 0, 0, 0, 0,
		stipple_width, stipple_height);
	XFree(image);
	XFreeGC(p->display, gc);
}

getFonts(p, prog)
register struct player	*p;
	char	*prog;
{
    char	*font_name;
    XFontStruct *XLoadQueryFont();
    XGCValues	gcv;

    if ((font_name = XGetDefault(p->display, prog, "font")) == NULL)
	font_name = "6x10";
    if ((p->dfont = XLoadQueryFont(p->display, font_name))
      == (XFontStruct *)NULL) {
	perror(font_name);
	return (1);
    }
    gcv.font = p->dfont->fid;
    gcv.foreground = p->textColor;
    gcv.background = p->backColor;
    p->dfgc = XCreateGC(p->display, p->w, GCForeground|GCBackground|GCFont, &gcv);
    if ((font_name = XGetDefault(p->display, PROGRAM_NAME, "boldFont")) == NULL)
	font_name = "6x10b";
    if ((p->bfont = XLoadQueryFont(p->display, font_name))
      == (XFontStruct *)NULL)
	p->bfont = p->dfont;
    gcv.font = p->bfont->fid;
    p->bfgc = XCreateGC(p->display, p->w, GCForeground|GCBackground|GCFont, &gcv);

    if ((font_name = XGetDefault(p->display, PROGRAM_NAME, "italicFont")) == NULL)
	font_name = "6x10i";
    if ((p->ifont = XLoadQueryFont(p->display, font_name))
      == (XFontStruct *)NULL)
	p->ifont = p->dfont;
    gcv.font = p->ifont->fid;
    p->ifgc = XCreateGC(p->display, p->w, GCForeground|GCBackground|GCFont, &gcv);

    if ((font_name = XGetDefault(p->display, PROGRAM_NAME, "bigFont")) == NULL)
	font_name = "-bitstream-charter-medium-r-normal-*-240-*";
    if ((p->bigFont = XLoadQueryFont(p->display, font_name))
      == (XFontStruct *)NULL)
	p->bigFont = p->dfont;
    gcv.font = p->bigFont->fid;
    p->bFgc = XCreateGC(p->display, p->w, GCForeground|GCBackground|GCFont, &gcv);

    if ((font_name = XGetDefault(p->display, PROGRAM_NAME, "xtrekFont")) == NULL)
	font_name = "xtrek";
    if ((p->xfont = XLoadQueryFont(p->display, font_name))
      == (XFontStruct *)NULL) {
	return (1);
    }
    gcv.font = p->xfont->fid;
    p->xfgc = XCreateGC(p->display, p->w, GCForeground|GCBackground|GCFont, &gcv);
    gcv.foreground = p->backColor;
    gcv.background = p->textColor;
    gcv.line_style = LineDoubleDash;
    gcv.dashes = 1;
    gcv.fill_style = FillStippled;
    gcv.stipple = XCreatePixmapFromBitmapData(p->display, RootWindow(p->display, p->screen), grey, grey_width, grey_height, 1, 0, 1);
    p->dimgc = XCreateGC(p->display, p->w, GCStipple|GCFillStyle|GCLineStyle|GCDashList|GCForeground|GCBackground|GCFont, &gcv);

    return (0);
}

redrawFed(p, fwin, flg)
register struct player	*p;
Window fwin;
{
    register int cen;
    char buf[BUFSIZ];
    static int numfeds = -1;

    if (numfeds == -1 || flg || numfeds != numShips(FED)) {
	XClearWindow(p->display, fwin);
	cen = xboxsize / 2 - (XTextWidth(p->dfont, "Federation", 10) / 2);
	XDrawImageString(p->display, fwin, p->dfgc, cen, p->dfont->ascent, "Federation", 10);
	(void) sprintf(buf, "%d", numfeds = numShips(FED));
	cen = xboxsize / 2 - (XTextWidth(p->bigFont, buf, strlen(buf)) / 2);
	XDrawString(p->display, fwin, p->bFgc, cen, p->bigFont->ascent + p->dfont->ascent + 15, buf, strlen(buf));
    }
}

redrawRom(p, rwin, flg)
register struct player	*p;
	Window rwin;
{
    register int cen;
    char buf[BUFSIZ];
    static int numroms = -1;

    if (numroms == -1 || flg || numroms != numShips(ROM)) {
	XClearWindow(p->display, rwin);
	cen = xboxsize / 2 - (XTextWidth(p->dfont, "Romulan", 7) / 2);
	XDrawImageString(p->display, rwin, p->dfgc, cen, p->dfont->ascent,
	    "Romulan", 7);
	(void) sprintf(buf, "%d", numroms = numShips(ROM));
	cen = xboxsize / 2 - (XTextWidth(p->bigFont, buf, strlen(buf)) / 2);
	XDrawString(p->display, rwin, p->bFgc, cen, p->bigFont->ascent + p->dfont->ascent + 15, buf, strlen(buf));
    }
}

redrawKli(p, kwin, flg)
register struct player	*p;
	Window kwin;
{
    register int cen;
    char buf[BUFSIZ];
    static int numklis = -1;

    if (numklis == -1 || flg || numklis != numShips(KLI)) {
	XClearWindow(p->display, kwin);
	cen = xboxsize / 2 - (XTextWidth(p->dfont, "Klingon", 7) / 2);
	XDrawImageString(p->display, kwin, p->dfgc, cen, p->dfont->ascent,
	    "Klingon", 7);
	(void) sprintf(buf, "%d", numklis = numShips(KLI));
	cen = xboxsize / 2 - (XTextWidth(p->bigFont, buf, strlen(buf)) / 2);
	XDrawString(p->display, kwin, p->bFgc, cen, p->bigFont->ascent + p->dfont->ascent + 15, buf, strlen(buf));
    }
}

redrawOri(p, owin, flg)
register struct player	*p;
	Window owin;
{
    register int cen;
    char buf[BUFSIZ];
    static int numoris = -1;

    if (numoris == -1 || flg || numoris != numShips(ORI)) {
	XClearWindow(p->display, owin);
	cen = xboxsize / 2 - (XTextWidth(p->dfont, "Orion", 5) / 2);
	XDrawImageString(p->display, owin, p->dfgc, cen, p->dfont->ascent,
	    "Orion", 5);
	(void) sprintf(buf, "%d", numoris = numShips(ORI));
	cen = xboxsize / 2 - (XTextWidth(p->bigFont, buf, strlen(buf)) / 2);
	XDrawString(p->display, owin, p->bFgc, cen, p->bigFont->ascent + p->dfont->ascent + 15, buf, strlen(buf));
    }
}

redrawQuit(p, qwin)
register struct player	*p;
	Window qwin;
{
    XDrawImageString(p->display, qwin, p->dfgc, p->dfont->ascent, p->dfont->ascent, "Quit xtrek", 10);
}

char *help_message[] = {
    "0-9  Set speed",
    "X    Add 10 to next key for speed",
    "k    Set course",
    "p    Fire phaser",
    "t    Launch torp",
    "d    detonate other torps",
    "D    detonate your torps",
    "+    Put up screens",
    "-    Put down screens",
    "u    Toggle screens",
    "b    Bomb planet",
    "z    Beam up armies",
    "x    Beam down armies",
    "R    Enter repair mode",
    "o    orbit planet",
    "Q    Quit",
    "?    Review messages",
    "/    Stop message Review",
    "c    Toggle cloak mode",
    "C    Coup a planet",
    "l    Lock on to player/planet",
    "@    (Dis)Allow copilots",
    "L    List players",
    "P    List planets",
    "S    List scores",
    "s    (Un)Map status window",
    "T	  Toggle Tractor beam",
    "U    Toggle show shields",
    "M    Turn on/off map window updating",
    "N    Turn on/off name mode",
    "i    Get info on player/planet",
    "h    (Un)Map this window",
    "w    (Un)Map war window",
    "*    start a practice robot",
    "&    send in a harder robot",
    "^    send in a very hard robot",
    0,
};

#define MAXHELP 40

fillhelp(p)
register struct player	*p;
{
    register int i = 0, row, column;

    for (column = 0; column < 4; column++) {
	for (row = 1; row < 9; row++) {
	    if (help_message[i] == 0)
		break;
	    else {
		XDrawImageString(p->display, p->helpWin, p->dfgc, fontWidth(p->dfont) * (MAXHELP * column + 1),
		    fontHeight(p->dfont) * row + p->dfont->ascent,
		    help_message[i], strlen(help_message[i]));
		i++;
	    }
	}
	if (help_message[i] == 0)
	    break;
    }
}

drawIcon(p)
register struct player	*p;
{
	if (debug)
		fprintf(stderr, "Drawing Icon Window for Player %d\n", p->p_no);
	XSetForeground(p->display, p->bmgc, p->textColor);
	XCopyPlane(p->display, p->ibm, p->iconWin, p->bmgc, 0, 0, icon_width, icon_height,
		0, 0, 1);
}

makeClock(p, w)
register struct player	*p;
	Window	w;
{
	int clky, clkx;

	xboxsize = p->p_xwinsize < 100 ? p->p_xwinsize : 100;
	yboxsize = p->p_ywinsize < 100 ? p->p_ywinsize : 100;

	clkx = (xboxsize / 2) - (clock_width / 2);
	clky = (yboxsize / 2) - (clock_height / 2);

	p->once = 0;
	p->oldtime = -1;
	p->clockw = XCreateWindow(p->display, w, clkx, clky,
		(unsigned) clock_width, (unsigned) (clock_height+16),
		0, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	XSetWindowBackground(p->display, p->clockw, p->backColor);
	XSetWindowBorder(p->display, p->clockw, p->backColor);
	XMapWindow(p->display, p->clockw);
	p->tbm = XCreateBitmapFromData(p->display, p->clockw, clock_bits, clock_width, clock_height);

	XClearWindow(p->display, p->clockw);
}

destroyClock(p)
register struct player	*p;
{
	XFreePixmap(p->display, p->tbm);
	p->tbm = (Pixmap) NULL;
	XDestroyWindow(p->display, p->clockw);
	p->clockw = (Window) NULL;
}

#define PI		3.141592654

showTimeLeft(p, time, max, flg)
register struct player	*p;
{
	char	buf[BUFSIZ], *cp;
	int	cx, cy, ex, ey, tx, ty;

	if (!p->once || flg) {
		p->once = 1;
		XClearWindow(p->display, p->clockw);
		p->oldtime = -1;
		cx = clock_width / 2;				/* 45 */
		cy = ((clock_height+16) - fontHeight(p->dfont)) / 2;	/* 26 */
		ex = cx - clock_width / 2;			/* 21 */
		ey = cy - (clock_height+16) / 2;			/* 2 */

		XSetForeground(p->display, p->bmgc, p->textColor);
		XCopyPlane(p->display, p->tbm, p->clockw, p->bmgc, 0, 0, clock_width, clock_height,
			ex, ey, 1);

		cp = "Auto Quit";
		tx = cx - (XTextWidth(p->dfont, cp, strlen(cp)) / 2);
		ty = clock_height - p->dfont->descent;
		XDrawImageString(p->display, p->clockw, p->dfgc, tx, ty, cp, strlen(cp));
	}

	XSetFunction(p->display, p->dfgc, GXinvert);
	if (p->oldtime != -1) {
		cx = clock_width / 2;				/* 45 */
		cy = (clock_height - fontHeight(p->dfont)) / 2;	/* 26 */
		ex = cx - clock_width * sin(2 * PI * p->oldtime / max) / 2;
		ey = cy - clock_height * cos(2 * PI * p->oldtime / max) / 2;
		XDrawLine(p->display, p->clockw, p->dfgc, cx, cy, ex, ey);
	}
	p->oldtime = time;

	cx = clock_width / 2;				/* 45 */
	cy = (clock_height - fontHeight(p->dfont)) / 2;	/* 26 */
	ex = cx - clock_width * sin(2 * PI * time / max) / 2;
	ey = cy - clock_height * cos(2 * PI * time / max) / 2;
	XDrawLine(p->display, p->clockw, p->dfgc, cx, cy, ex, ey);
	XSetFunction(p->display, p->dfgc, GXcopy);

	sprintf(buf, "%2.2d", max - time);
	tx = cx - (XTextWidth(p->dfont, buf, strlen(buf)) / 2);
	ty = cy - fontHeight(p->dfont) / 2;
	XDrawImageString(p->display, p->clockw, p->dfgc, tx, ty, buf, strlen(buf));
}
