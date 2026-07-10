static char sccsid[] = "@(#)planetlist.c	3.1";

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
#include "defs.h"
#include "data.h"

static char *teamname[8] = {
    "XXX",
    "FED",
    "ROM",
    "KLI",
    "ORI",
    "IND",
    "NOT",
    "GOD"
};

/*
** Open a window which contains all the planets and their current
** statistics.  Players will not know about planets that their team
** has not orbited.
*/

planetlist(p)
register struct player	*p;
{
    register int i;
    register int k = 0;
    char buf[BUFSIZ];
    char buf2[BUFSIZ];
    register struct planet *j;

    (void) sprintf(buf, "  # Planet Name      own armies type");
    if (isGod(p))
	strcat(buf, "   OVel  OAng  SOAng");
    XDrawImageString(p->display, p->planetw, p->dfgc, 0, p->dfont->ascent, buf,
       strlen(buf));
    k = 2;
    for (i = 0, j = &planets[0]; i < MAXPLANETS; i++, j++) {
	if (j->pl_flags & PLINVIS)
		continue;
	if (j->pl_info & (1 << p->p_ship->s_team)) {
	    (void) sprintf(buf, " %2d %-16s %3s %3d    %6s",
		j->pl_no,
		j->pl_name,
		teamname[j->pl_owner],
		j->pl_owner <= SELFRULED ? j->pl_armies : 0,
		j->pl_tname);
	}
	else {
	    (void) sprintf(buf, " %2d %-16s",
		j->pl_no,
		j->pl_name);
	}
	if (isGod(p)) {
	    sprintf(buf2, " %3d    %3d   %d", j->pl_orbvel, j->pl_orbang, j->pl_suborbang);
	    strcat(buf, buf2);
	}
	XDrawImageString(p->display, p->planetw, p->dfgc, 0, p->dfont->ascent + fontHeight(p->dfont) * k++, buf, strlen(buf));
    }
}
