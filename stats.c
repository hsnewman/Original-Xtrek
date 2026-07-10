static char sccsid[] = "@(#)stats.c	3.1";
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
#include "defs.h"
#include "data.h"

#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))

/* NOTE: These should be set in player data? */
#define	BX_OFF()	((p->textWidth + 1) * fontWidth(p->dfont) + S_IBORDER)
#define	BY_OFF(line)	((line) * (fontHeight(p->dfont) + S_IBORDER) + S_IBORDER)
#define	TX_OFF(len)	((p->textWidth - len) * fontWidth(p->dfont) + S_IBORDER)
#define	TY_OFF(line)	BY_OFF(line)

#define STAT_WIDTH		160
#define STAT_HEIGHT		BY_OFF(NUM_SLIDERS)
#define STAT_BORDER		2
#define S_IBORDER		5
#define STAT_X			422
#define STAT_Y			13

#define SL_WID			\
	(STAT_WIDTH - 2 * S_IBORDER - (p->textWidth + 1) * fontWidth(p->dfont))
#define SL_HEI			(fontHeight(p->dfont))

#define NUM_ELS(a)		(sizeof (a) / sizeof (*(a)))
#define NUM_SLIDERS		NUM_ELS(sliders)

typedef struct slider {
	char	*label;
	int	min, max;
	int	low_red, high_red;
	int	label_length;
	int	diff;
} SLIDER;

static SLIDER	sliders[] = {
	{ "Shields",		0,	100,	20,	100	},
	{ "Damage",		0,	100,	0,	0	},
	{ "Fuel",		0,	10000,	1000,	10000	},
	{ "Warp",		0,	9,	0,	9	},
	{ "Weapon Temp",	0,	1200,	0,	1000	},
	{ "Engine Temp",	0,	1200,	0,	1000	},
};

initStats(p, prog)
register struct player	*p;
	char	*prog;
{
	int	i;
	char	*str;
	unsigned int dummy1, dummy2;

	if ((str = XGetDefault(p->display, prog, "stats.geometry")) != NULL) {
		p->uspec = XParseGeometry(str, &p->statX, &p->statY, &dummy1, &dummy2);
		if ((p->uspec & (XValue|XNegative)) == (XValue|XNegative))
			p->statX = -(p->statX);
		if ((p->uspec & (YValue|YNegative)) == (YValue|YNegative))
			p->statY = -(p->statY);
	}
	for (i = 0; i < NUM_SLIDERS; i++) {
		sliders[i].label_length = strlen(sliders[i].label);
		p->textWidth = MAX(p->textWidth, sliders[i].label_length);
		sliders[i].diff = sliders[i].max - sliders[i].min;
	}
}

Window
openStats(p)
struct player	*p;
{
	Window		w;
	extern char 	*calloc();
	XSizeHints	wininfo;
	XGCValues	values;

	if (p->statwin) {
		closeStats(p, p->statwin);
	}
	p->p_flags |= PFSHOWSTATS;

	w = XCreateWindow(p->display, RootWindow(p->display, p->screen), p->statX, p->statY,
		(unsigned) STAT_WIDTH, (unsigned) STAT_HEIGHT, STAT_BORDER, DefaultDepth(p->display, p->screen), InputOutput,
		(Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
	(void) XSetWMProtocols(p->display, w, &p->wm_delete_window, 1);
	XDefineCursor(p->display, w, (Cursor) XCreateFontCursor(p->display, XC_crosshair));
	wininfo.x = p->statX;
	wininfo.y = p->statY;
	wininfo.width = STAT_WIDTH;
	wininfo.height = STAT_HEIGHT;
	wininfo.min_width = STAT_WIDTH;
	wininfo.min_height = STAT_HEIGHT;
	wininfo.max_width = STAT_WIDTH;
	wininfo.max_height = STAT_HEIGHT;
	if (p->uspec & (XValue|YValue|XNegative|YNegative))
		wininfo.flags = USPosition | PSize | PMinSize | PMaxSize;
	else
		wininfo.flags = PPosition | PSize | PMinSize | PMaxSize;
	XSetNormalHints(p->display, w, &wininfo);
	XStoreName(p->display, w, "xtrek-stats");
	XSetWindowBackground(p->display, w, p->backColor);
	XSetWindowBorder(p->display, w, p->borderColor);
	values.foreground = p->myColor;
	values.background = p->backColor;
	p->sgc = XCreateGC(p->display, w, GCForeground|GCBackground, &values);
	p->p_rp = (RECORD *) calloc(NUM_SLIDERS, sizeof (RECORD));
	p->p_rp[0].data = &(p->p_ship->s_shield);
	p->p_rp[1].data = &(p->p_ship->s_damage);
	p->p_rp[2].data = &(p->p_ship->s_fuel);
	p->p_rp[3].data = &(p->p_ship->s_speed);
	p->p_rp[4].data = &(p->p_ship->s_wtemp);
	p->p_rp[5].data = &(p->p_ship->s_etemp);
	XSelectInput(p->display, w, ExposureMask|SubstructureNotifyMask);
	XMapWindow(p->display, w);
	return (w);
}

redrawStats(p, w)
register struct player	*p;
Window	w;
{
	int	i;

	if (p->p_rp == NULL) {
		fputs("You gave redrawStats a bum window\n", stderr);
		return;
	}
	XClearWindow(p->display, w);
	for (i = 0; i < NUM_SLIDERS; i++) {
		p->p_rp[i].last_value = 0;
		XSetForeground(p->display, p->sgc, p->myColor);
		XDrawImageString(p->display, w, p->sgc, TX_OFF(sliders[i].label_length), TY_OFF(i) + p->dfont->ascent,
			sliders[i].label, sliders[i].label_length);
		box(p, w, 0, BX_OFF() - 1, BY_OFF(i) - 1, SL_WID+1, SL_HEI+1, p->myColor);
	}
}

closeStats(p, w)
register struct player	*p;
	Window	w;
{
	XWindowAttributes wa;

	XFreeGC(p->display, p->sgc);
	XGetWindowAttributes(p->display, w, &wa);
	p->statX = wa.x;
	p->statY = wa.y;
	XDestroyWindow(p->display, w);
	p->p_flags &= ~PFSHOWSTATS;
	p->statwin = (Window) NULL;
}

updateStats(p, w)
register struct player *p;
	Window		w;
{
	int	i, value, diff, old_x, new_x, wid;
	unsigned long	color;
	RECORD	*r;
	SLIDER	*s;

	for (i = 0; i < NUM_SLIDERS; i++) {
		r = &p->p_rp[i];
		s = &sliders[i];
		value = *(r->data);
		if (value < s->min)
			value = s->min;
		else if (value > s->max)
			value = s->max;
		if (value == r->last_value)
			continue;
		diff = value - r->last_value;
		color = p->myColor;
		if (value < s->low_red)
			color = p->warningColor;
		else if (value > s->high_red)
			color = p->warningColor;
		old_x = r->last_value * SL_WID / s->diff;
		new_x = value * SL_WID / s->diff;
		wid = new_x - old_x;
		if (wid == 0)
			continue;
		if (wid < 0)
			wid = -wid;
		if (new_x > 0)
			box(p, w, 1, BX_OFF(), BY_OFF(i), new_x, SL_HEI, color);
		if (diff < 0)
			cbox(p, w, BX_OFF() + new_x, BY_OFF(i), wid, SL_HEI);
		r->last_value = value;
	}
}

/*ARGSUSED*/
box(p, w, filled, x, y, width, height, color)
register struct player	*p;
Window	w;
int	filled;
int	x, y;
unsigned width, height;
unsigned long	color;
{
	XSetForeground(p->display, p->sgc, color);
	if (filled)
		XFillRectangle(p->display, w, p->sgc, x, y, width, height);
	else
		XDrawRectangle(p->display, w, p->sgc, x, y, width, height);
}

cbox(p, w, x, y, width, height)
register struct player	*p;
Window	w;
int	x, y;
unsigned width, height;
{
	XClearArea(p->display, w, x, y, width, height, False);
}
