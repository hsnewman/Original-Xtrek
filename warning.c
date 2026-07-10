static char sccsid[] = "@(#)warning.c	3.1";
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
** The warning in text will be printed in the warning window.
*/
warning(p, text)
register struct player	*p;
register char		*text;
{
    register struct player	*co;
    register int		i;

    if (p->p_flags & PFROBOT)
	return;

    for (i = 0, co = &players[0]; i < MAXPLAYER; i++, co++) {
	if (co->p_status != PALIVE || co->p_ship != p->p_ship)
		continue;
	if (co->p_flags & PFROBOT)
		continue;
	co->p_warntimer = udcounter + WARNTIME;  /* WARNTIME updates later the line will be cleared */
	if (co->p_warncount > 0)
		XFillRectangle(co->display, co->warnw, co->cleargc,
			5, 5, co->p_warncount, fontHeight(co->dfont));
	XDrawImageString(co->display, co->warnw, co->dfgc,
		5, 5 + co->dfont->ascent, text, strlen(text));
	p->p_warncount = XTextWidth(p->dfont, text, strlen(text));
    }
}
