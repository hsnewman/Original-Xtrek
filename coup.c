static char sccsid[] = "@(#)coup.c	3.1";
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

/* Cray X11/Xlib.h includes sys/types.h */
#if !defined(cray)
#include <sys/types.h>
#endif
#include <stdio.h>
#include "defs.h"
#include "data.h"

/* throw a coup */

coup(p)
register struct player	*p;
{
    register int i;
    register struct planet *l;

    if (p->p_ship->s_stats.st_kills < 1.0) {
	warning(p, "You must have one kill to throw a coup");
	return;
    }
    if (!(p->p_ship->s_flags & SFORBIT)) {
	warning(p, "You must orbit your home planet to throw a coup");
	return;
    }
    for (i = 0, l = &planets[0]; i < MAXPLANETS; i++, l++) {
	if ((l->pl_owner == p->p_ship->s_team) && (l->pl_armies > 0)) {
	    warning(p, "You already own a planet!!!");
	    return;
	}
    }
    l = &pdata[p->p_ship->s_planet];	/* Original planet info */

    if ((!(l->pl_flags & PLHOME)) || (l->pl_owner != p->p_ship->s_team)) {
	warning(p, "You must orbit your home planet to throw a coup");
	return;
    }

    l = &planets[p->p_ship->s_planet];	/* Planet player is orbiting...current info */

    if (l->pl_armies > 4) {
	warning(p, "Too many armies on planet to throw a coup");
	return;
    }

    if (l->pl_couptime > 0) {
	warning(p, "Planet not yet ready for a coup");
	return;
    }

    if (l->pl_flags & PLCOUP) { /* Avoid race conditions */
	return;
    }

    /* the cases are now met.  We can have a coup. */

    l->pl_flags |= PLCOUP;
    p->p_ship->s_stats.st_coups++;
}
