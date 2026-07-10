static char sccsid[] = "@(#)playerlist.c	3.1";

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

playerlist(p, fid)
register struct player	*p;
int				fid;
{
    register int i;
    register int k = 0;
    char buf[BUFSIZ];
    register struct player *j;

    (void) sprintf(buf, " # Ship Name             Login    Display          dir spd kills");
    if (p) {
	XDrawImageString(p->display, p->playerw, p->dfgc, 0, p->dfont->ascent, buf, strlen(buf));
	k = 2;
    } else {
	write(fid, buf, strlen(buf));
	write(fid, "\n", 1);
    }

    for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	if (j->p_status != PALIVE)
	    continue;
	(void) sprintf(buf, " %1x (%1c%1x) %-16.16s %-8s %-16.16s %3d %3d %5.2f",
	    j->p_no,
	    teamlet[j->p_ship->s_team],
	    j->p_ship->s_no,
	    j->p_name,
	    j->p_login,
	    j->p_monitor,
	    j->p_ship->s_dir,
	    j->p_ship->s_speed,
	    j->p_ship->s_stats.st_kills);
	if (p) {
		XDrawImageString(p->display, p->playerw, p->dfgc, 0, p->dfont->ascent + fontHeight(p->dfont) * k++, buf, strlen(buf));
	} else {
		write(fid, buf, strlen(buf));
		write(fid, "\n", 1);
	}
    }
}
