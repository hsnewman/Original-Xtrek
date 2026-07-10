static char sccsid[] = "@(#)util.c	3.1";

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

/*
** Provide the angular distance between two angles.
*/
angdist(x, y)
unsigned char x, y;
{
    register unsigned char res;

    res = ABS(x - y);
    if (res > 128)
	return(256 - (int) res);
    return((int) res);
}

/*
** Find the object nearest mouse.  Returns a pointer to an
** obtype structure.  This is used for info, locking on, and towing.
**
** Because we are never interested in it, this function will
** never return your own ship as the target.
**
** Finally, this only works on the two main windows
*/

static struct obtype _target;

struct obtype *
gettarget(p, ww, x, y, targtype)
register struct player	*p;
Window ww;
int x, y;
int targtype;
{
    register int i;
    register struct player *j;
    register struct planet *k;
    int	g_x, g_y;
    double dist, closedist;

    if (ww == p->mapw) {
	g_x = x * GWIDTH / p->p_xwinsize;
	g_y = y * GWIDTH / p->p_ywinsize;
    }
    else {
	g_x = p->p_ship->s_x + ((x - p->p_xwinsize/2) * SCALE);
	g_y = p->p_ship->s_y + ((y - p->p_ywinsize/2) * SCALE);
    }
    closedist = GWIDTH;

    if (targtype & TARG_PLANET) {
	for (i = 0, k = &planets[0]; i < MAXPLANETS; i++, k++) {
	    if (!isGod(p) && (k->pl_flags & PLINVIS))
		continue;
	    dist = hypot((double) (g_x - k->pl_x), (double) (g_y - k->pl_y));
	    if (dist < closedist) {
		_target.o_type = PLANETTYPE;
		_target.o_num = i;
		_target.o_dist = dist;
		closedist = dist;
	    }

	}
    }

    if (targtype & TARG_PLAYER) {
	for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	    if (j->p_status != PALIVE)
		continue;
	    if (!isGod(p) &&
	        ((j->p_ship->s_flags & SFCLOAK) && (!(j->p_ship->s_flags & SFDCLOAK))) &&
	        (!(targtype & TARG_CLOAK)))
		continue;
	    if (j == p && (targtype & TARG_MYSELF) == 0)
		continue;
	    dist = hypot((double) (g_x - j->p_ship->s_x), (double) (g_y - j->p_ship->s_y));
	    if (dist < closedist) {
		_target.o_type = PLAYERTYPE;
		_target.o_num = i;
		_target.o_dist = dist;
		closedist = dist;
	    }
	}
    }

    if (closedist == GWIDTH) {		/* Didn't get one.  bad news */
	_target.o_type = PLAYERTYPE;
	_target.o_num = p->p_ship->s_no;	/* Return myself.  Oh well... */
	_target.o_dist = 0;
	return(&_target);
    }
    else {
	return(&_target);
    }
}

iscloaked(s)
register struct ship	*s;
{
	return ((s->s_flags & SFCLOAK) && (!(s->s_flags & SFDCLOAK)));
}

/*
** Tell us if a window is currently visible on the screen
*/

ismapped(p, win)
register struct player	*p;
Window win;
{
    XWindowAttributes info;

    XGetWindowAttributes(p->display, win, &info);

    return (info.map_state == IsViewable);
}

#ifdef hpux
#include <sys/signal.h>

void (*
signal(sig, funct))()
int sig;
int (*funct)();
{
    struct sigvec vec;

/*
 *  jas (Jeff Schmidt) Nuke this.  'vec.sv_flags' gets set to SV_RESETHAND
 *  when the following is invoked on the HP 9000/300.

    sigvector(sig, 0, &vec);
 */
    vec.sv_handler = funct;
    vec.sv_flags = 0L;
    sigvector(sig, &vec, (struct sigvec *) 0);
}
#endif hpux
