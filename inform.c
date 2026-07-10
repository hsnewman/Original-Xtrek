static char sccsid[] = "@(#)inform.c	3.1";
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
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "defs.h"
#include "data.h"

/* Display information about the nearest objext to mouse */

/*
** When the player asks for info, this routine finds the object
** nearest the mouse, either player or planet, and pops up a window
** with the desired information in it.
**
** We intentionally provide less information than is actually
** available.  Keeps the fog of war up.
**
** There is a different sized window for each type player/planet
** and we take care to keep it from extending beyond the main
** window boundaries.
*/

inform(p, ww, x, y)
register struct player	*p;
Window ww;
int x, y;
{
    int mx, my;
    int rx, ry;
    int		maskr;
    Window subw;
    struct obtype *gettarget(), *target;
    XWindowAttributes	wa;

    p->p_infomapped = 1;
    target = gettarget(p, ww, x, y, TARG_PLAYER|TARG_PLANET|TARG_MYSELF);
    p->p_info_target = *target;

    XGetWindowAttributes(p->display, ww, &wa);

    XQueryPointer(p->display, ww, &subw, &subw, &rx, &ry, &mx, &my, &maskr);

    if (target->o_type == PLAYERTYPE) {
	/* Too close to the edge? */
	if (mx + 23 * fontWidth(p->dfont) + 2 > wa.width)
	    mx = wa.width - 23 * fontWidth(p->dfont) - 2;
	if (my + 7 * fontHeight(p->dfont) + 2 > wa.height)
	    my = wa.height - 7 * fontHeight(p->dfont) - 2;

	p->infow = XCreateWindow(p->display, ww, mx, my, (unsigned) 23 * fontWidth(p->dfont),
	    (unsigned) 7 * fontHeight(p->dfont), 2, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    } else {  /* Planet */
	/* Too close to the edge? */
	if (mx + 20 * fontWidth(p->dfont) + 2 > wa.width)
	    mx = wa.width - 20 * fontWidth(p->dfont) - 2;
	if (my + 3 * fontHeight(p->dfont) + 2 > wa.height)
	    my = wa.height - 3 * fontHeight(p->dfont) - 2;

	p->infow = XCreateWindow(p->display, ww, mx, my, (unsigned) 20 * fontWidth(p->dfont),
	    (unsigned) 3 * fontHeight(p->dfont), 2, DefaultDepth(p->display, p->screen), InputOutput, (Visual *)CopyFromParent, 0L, (XSetWindowAttributes *)0);
    }
    XSelectInput(p->display, p->infow, ExposureMask|SubstructureNotifyMask);
    XSetWindowBackground(p->display, p->infow, p->backColor);
    XClearWindow(p->display, p->infow);
    XMapWindow(p->display, p->infow);
}

drawinfo(p)
register struct player	*p;
{
    register struct obtype	*target;
    register struct player	*j;
    register struct planet	*k;
    char buf[BUFSIZ*2];
    int line = 0;
    double dist;

    if (!p->p_infomapped) {
	fprintf(stderr, "drawinfo: Player %d no info window\n", p->p_no);
    }
    target = &p->p_info_target;
    if (target->o_type == PLAYERTYPE) {
	j = &players[target->o_num];
	dist = hypot((double) (p->p_ship->s_x - j->p_ship->s_x),
	    (double) (p->p_ship->s_y - j->p_ship->s_y));
	(void) sprintf(buf, "%s (%c%1x):", j->p_name,
		teamlet[j->p_ship->s_team], j->p_ship->s_no);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	(void) sprintf(buf, "Login   %-s", j->p_login);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	(void) sprintf(buf, "Display %-s", j->p_monitor);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	(void) sprintf(buf, "Speed   %-d", j->p_ship->s_speed);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));

	/*  Too much information.  Let 'em wonder	NOTE: Give info to team members
	(void) sprintf(buf, "Damage  %-d", j->p_ship->s_damage);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	(void) sprintf(buf, "Shields %-d", j->p_ship->s_shield);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	*/

	(void) sprintf(buf, "kills   %-4.2f", j->p_ship->s_stats.st_kills);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	(void) sprintf(buf, "dist    %-d", (int) dist);
	XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	if (j->p_ship->s_swar & (1 << p->p_ship->s_team))
	    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, "WAR", 3);
	else if (j->p_ship->s_hostile & (1 << p->p_ship->s_team))
	    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, "HOSTILE", 7);
	else
	    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, "PEACEFUL", 8);
    } else {  /* Planet */
	k = &planets[target->o_num];
	dist = hypot((double) (p->p_ship->s_x - k->pl_x),
	    (double) (p->p_ship->s_y - k->pl_y));
	if (k->pl_info & (1 << p->p_ship->s_team)) {
	    (void) sprintf(buf, "%s (%c)", k->pl_name, teamlet[k->pl_owner]);
	    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	    if (k->pl_type != SUN) {
		    (void) sprintf(buf, "Armies %d", k->pl_armies);
		    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
			p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	    }
	    (void) sprintf(buf, "%c%c%c%c  (%s)",
		(k->pl_info & (1 << FED) ? 'F' : ' '),
		(k->pl_info & (1 << ROM) ? 'R' : ' '),
		(k->pl_info & (1 << KLI) ? 'K' : ' '),
		(k->pl_info & (1 << ORI) ? 'O' : ' '),
		k->pl_tname);
	    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	}
	else {
	    (void) sprintf(buf, "%s", k->pl_name);
	    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	    (void) sprintf(buf, "No other info");
	    XDrawImageString(p->display, p->infow, p->dfgc, fontWidth(p->dfont),
		p->dfont->ascent + fontHeight(p->dfont) * line++, buf, strlen(buf));
	}
    }
}


destroyInfo(p)
register struct player	*p;
{
    XDestroyWindow(p->display, p->infow);
    p->infow = (Window) NULL;
    p->p_infomapped = 0;
}
