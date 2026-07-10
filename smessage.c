static char sccsid[] = "@(#)smessage.c	3.1";

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
#include <ctype.h>
#include "defs.h"
#include "data.h"


smessage(p, ichar)
register struct player	*p;
char ichar;
{
    register int i;

    if (p->p_umsg.m_pending == 0) {
	p->p_umsg.m_pending = 1;
	if (p->p_mdisplayed) {
		XFillRectangle(p->display, p->messagew, p->cleargc, 5 + fontWidth(p->dfont), 5,
		    p->p_lastcount, fontHeight(p->dfont));
	    p->p_mdisplayed = 0;
	}
	/* Put the proper recipient in the window */
	if (getaddr(p, ichar) < 0) {
	    /* print error message */
	    p->p_umsg.m_pending = 0;
	    return;
	}
	XDrawImageString(p->display, p->messagew, p->dfgc, 5 + fontWidth(p->dfont), 5 + p->dfont->ascent,
	    p->p_umsg.m_addrmsg, UMSGADDRLEN);
	p->p_umsg.m_lcount = UMSGADDRLEN;
	return;
    }
    switch (ichar) {
	case '\b':
	case '\177':
	    if (--p->p_umsg.m_lcount < UMSGADDRLEN) {
		p->p_umsg.m_lcount = UMSGADDRLEN;
		break;
	    }
		XFillRectangle(p->display, p->messagew, p->cleargc, 5 + fontWidth(p->dfont) * p->p_umsg.m_lcount, 5,
		    fontWidth(p->dfont), fontHeight(p->dfont));
	    break;
	case '\027':	/* CTRL-w */
	    i = 0;
	    /* back up over blanks */
	    while (--p->p_umsg.m_lcount >= UMSGADDRLEN && isspace(p->p_umsg.m_buf[p->p_umsg.m_lcount - UMSGADDRLEN]))
		i++;
	    p->p_umsg.m_lcount++;
	    /* back up over non-blanks */
	    while (--p->p_umsg.m_lcount >= UMSGADDRLEN && !isspace(p->p_umsg.m_buf[p->p_umsg.m_lcount - UMSGADDRLEN]))
		i++;
	    p->p_umsg.m_lcount++;

	    if (i > 0) {
		XFillRectangle(p->display, p->messagew, p->cleargc, 5 + fontWidth(p->dfont) * p->p_umsg.m_lcount, 5,
		    fontWidth(p->dfont) * i, fontHeight(p->dfont));
	    }
	    break;
	case '\025':	/* CTRL-u */
	case '\030':	/* CTRL-x */
		    while (--p->p_umsg.m_lcount >= UMSGADDRLEN)
			XFillRectangle(p->display, p->messagew, p->cleargc, 5 + fontWidth(p->dfont) * UMSGADDRLEN, 5,
			    fontWidth(p->dfont) * (p->p_umsg.m_lcount - UMSGADDRLEN), fontHeight(p->dfont));
	       p->p_umsg.m_pending = 0;
	    break;
	case '\033':	/* ESC */
		XFillRectangle(p->display, p->messagew, p->cleargc, 5, 5,
		    fontWidth(p->dfont) * p->p_umsg.m_lcount,
		    fontHeight(p->dfont));
		p->p_mdisplayed = 0;
		p->p_umsg.m_pending = 0;
	    break;
	case '\r':
	    p->p_umsg.m_buf[p->p_umsg.m_lcount - UMSGADDRLEN] = 0;
	    p->p_umsg.m_pending = 0;
	    switch (p->p_umsg.m_addr) {
		case 'A':
		    pmessage(p->p_umsg.m_buf, 0, MALL, p->p_umsg.m_addrmsg);
		    break;
		case 'F':
		case 'f':
		    pmessage(p->p_umsg.m_buf, FED, MTEAM, p->p_umsg.m_addrmsg);
		    break;
		case 'R':
		case 'r':
		    pmessage(p->p_umsg.m_buf, ROM, MTEAM, p->p_umsg.m_addrmsg);
		    break;
		case 'K':
		case 'k':
		    pmessage(p->p_umsg.m_buf, KLI, MTEAM, p->p_umsg.m_addrmsg);
		    break;
		case 'O':
		case 'o':
		    pmessage(p->p_umsg.m_buf, ORI, MTEAM, p->p_umsg.m_addrmsg);
		    break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    pmessage(p->p_umsg.m_buf, p->p_umsg.m_addr - '0', MINDIV, p->p_umsg.m_addrmsg);
		    break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		    pmessage(p->p_umsg.m_buf, p->p_umsg.m_addr - 'a' - 10, MINDIV, p->p_umsg.m_addrmsg);
		    break;
		default:
		    warning(p, "Not legal recipient");
	    }
		XFillRectangle(p->display, p->messagew, p->cleargc, 5, 5,
		    fontWidth(p->dfont) * p->p_umsg.m_lcount, fontHeight(p->dfont));
		p->p_mdisplayed = 0;
	    p->p_umsg.m_lcount = 0;
	    break;
	default:
	    if (p->p_umsg.m_lcount == UMSGLEN) {
		XBell(p->display, p->screen);
		break;
	    }
	    if (iscntrl(ichar))
		break;
	    XDrawImageString(p->display, p->messagew, p->dfgc, 5 + fontWidth(p->dfont) * p->p_umsg.m_lcount, 5 + p->dfont->ascent,
			&ichar, 1);
	    p->p_umsg.m_buf[(p->p_umsg.m_lcount++) - UMSGADDRLEN] = ichar;
	    break;
    }
}

pmessage(str, recip, group, address)
char *str;
int recip;
int group;
char *address;
{
    struct message *cur;
    if (++(mctl->mc_current) >= MAXMESSAGE)
	mctl->mc_current = 0;
    cur = &messages[mctl->mc_current];
    cur->m_no = mctl->mc_current;
    cur->m_flags = group;
    cur->m_time = 0;
    cur->m_recpt = recip;
    (void) sprintf(cur->m_data, "%-9s %s", address, str);
    cur->m_flags |= MVALID;
}

getaddr(p, who)
register struct player	*p;
char who;
{
    p->p_umsg.m_addr = who;
    (void) sprintf(p->p_umsg.m_addrmsg, " %c%x->", teamlet[p->p_ship->s_team], p->p_ship->s_no);
    switch (who) {
	case 'A':
	    (void) sprintf(&p->p_umsg.m_addrmsg[5], "ALL");
	    break;
	case 'F':
	case 'f':
	    (void) sprintf(&p->p_umsg.m_addrmsg[5], "FED");
	    break;
	case 'R':
	case 'r':
	    (void) sprintf(&p->p_umsg.m_addrmsg[5], "ROM");
	    break;
	case 'K':
	case 'k':
	    (void) sprintf(&p->p_umsg.m_addrmsg[5], "KLI");
	    break;
	case 'O':
	case 'o':
	    (void) sprintf(&p->p_umsg.m_addrmsg[5], "ORI");
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    if (isAlive(&players[who - '0'])) {
		(void) sprintf(&p->p_umsg.m_addrmsg[5], "%c%x ",
		    teamlet[players[who - '0'].p_ship->s_team], who - '0');
	    }
	    else {
		warning(p, "Player is not in game");
		return(-1);
	    }
	    break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	    if (isAlive(&players[who - 'a' + 10])) {
		(void) sprintf(&p->p_umsg.m_addrmsg[5], "%c%x ",
		    teamlet[players[who - 'a' + 10].p_ship->s_team], who - 'a' + 10);
	    }
	    else {
		warning(p, "Player is not in game");
		return(-1);
	    }
	    break;
	default:
	    warning(p, "Not legal recipient");
	    return(-1);
    }
    return(0);
}
