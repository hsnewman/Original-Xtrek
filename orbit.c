static char sccsid[] = "@(#)orbit.c	3.1";

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
#if !defined(cray)
#include <sys/types.h>
#endif
#include "defs.h"
#include "data.h"

/* orbit a planet */

orbit(p)
register struct player	*p;
{
    register int i;
    register struct planet *l;
    unsigned char dir;
    int dx, dy;
    unsigned char iatan2();

    if (!isGod(p) && p->p_ship->s_speed > ORBSPEED) {
	warning(p, "Speed must be less than two to orbit");
	return;
    }
    if (p->p_ship->s_flags & SFORBIT) {
	warning(p, "You are already orbiting");
	return;
    }

    for (i = 0, l = &planets[0]; i < MAXPLANETS; i++, l++) {
	dx = ABS(l->pl_x - p->p_ship->s_x);
	dy = ABS(l->pl_y - p->p_ship->s_y);
	if (dx > ORBDIST || dy > ORBDIST)		/*XXX*/
	    continue;
	if (dx * dx + dy * dy > ORBDIST * ORBDIST)
	    continue;

	dir = iatan2(l->pl_x - p->p_ship->s_x, l->pl_y - p->p_ship->s_y);
	l->pl_info |= (1 << p->p_ship->s_team);
	p->p_ship->s_dir = dir + 64;
	p->p_ship->s_flags |= SFORBIT;
	p->p_ship->s_x = l->pl_x + ((ORBDIST * icos[dir]) >> TRIGSCALE);
	p->p_ship->s_y = l->pl_y + ((ORBDIST * isin[dir]) >> TRIGSCALE);
	p->p_ship->s_speed = p->p_ship->s_desspeed = 0;
	p->p_ship->s_planet = l->pl_no;
	return;
    }

    warning(p, "Not close enough to any planet to orbit");
}
