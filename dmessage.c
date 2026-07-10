static char sccsid[] = "@(#)dmessage.c	3.1";

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

dmessage(p)
register struct player	*p;
{
    struct message *cur;
    char buf[80];

/*    if (p->p_umsg.m_pending || (p->p_flags & PFENTER))*/
    if (p->p_umsg.m_pending)
	return;

    if (mctl->mc_current == p->p_lastm) {
	if (p->p_mdisplayed) {
	    XFillRectangle(p->display, p->messagew, p->cleargc, p->dfont->ascent, 5,
		p->p_lastcount, fontHeight(p->dfont));
	    p->p_mdisplayed = 0;
	}
	return;
    }
    do {
	if (++(p->p_lastm) >= MAXMESSAGE)
	    p->p_lastm = 0;
	cur = &messages[p->p_lastm];
	if (cur->m_flags & MVALID) {
	    if (isGod(p) || (cur->m_flags & MALL) ||
		((cur->m_flags & MTEAM) && (cur->m_recpt == p->p_ship->s_team)) ||
		((cur->m_flags & MINDIV) && (cur->m_recpt == p->p_ship->s_no))) {
		    if (p->p_mdisplayed) {
		        XFillRectangle(p->display, p->messagew, p->cleargc, p->dfont->ascent, 5,
			    p->p_lastcount, fontHeight(p->dfont));
			p->p_mdisplayed = 0;
		    }
		    (void) sprintf(buf, "%s", cur->m_data);
		    p->p_lastcount = XTextWidth(p->dfont, buf, strlen(buf));
		    XDrawImageString(p->display, p->messagew, p->dfgc, p->dfont->ascent, 5 + p->dfont->ascent, buf, strlen(buf));
		    XBell(p->display, p->screen);
		    p->p_mdisplayed = 1;
		    return;
	    }
	}
    } while (p->p_lastm != mctl->mc_current);
}
