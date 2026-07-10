static char sccsid[] = "@(#)war.c	3.1";
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

/* Set up the war window and map it */
static char *feds = "FED - ";
static char *roms = "ROM - ";
static char *klis = "KLI - ";
static char *oris = "ORI - ";
static char *gos = "  Re-program";
static char *exs = "  Exit - no change";
static char *peaces = "Peace";
static char *hostiles = "Hostile";
static char *wars = "War";

warwindow(p)
register struct player	*p;
{
    XMapWindow(p->display, p->war);
    p->p_ship->s_newhostile = p->p_ship->s_hostile;
    p->p_ship->s_reprogram = 0;
}

warrefresh(p)
register struct player	*p;
{
    warfed(p);
    warrom(p);
    warkli(p);
    warori(p);
    wargo(p);
    warno(p);
}

wargo(p)
register struct player	*p;
{
    XDrawImageString(p->display, p->wargo, p->dfgc, 0, fontHeight(p->dfont) / 2 + p->dfont->ascent, gos, strlen(gos));
}

warno(p)
register struct player	*p;
{
    XDrawImageString(p->display, p->warno, p->dfgc, 0, fontHeight(p->dfont) / 2 + p->dfont->ascent, exs, strlen(exs));
}

warfed(p)
register struct player	*p;
{
    fillwin(p, p->warf, feds, p->p_ship->s_newhostile, p->p_ship->s_swar, FED);
}

warrom(p)
register struct player	*p;
{
    fillwin(p, p->warr, roms, p->p_ship->s_newhostile, p->p_ship->s_swar, ROM);
}

warkli(p)
register struct player	*p;
{
    fillwin(p, p->wark, klis, p->p_ship->s_newhostile, p->p_ship->s_swar, KLI);
}

warori(p)
register struct player	*p;
{
    fillwin(p, p->waro, oris, p->p_ship->s_newhostile, p->p_ship->s_swar, ORI);
}

fillwin(p, win, string, hostile, warbits, team)
register struct player	*p;
Window win;
char *string;
int hostile, warbits;
int team;
{
    char buf[80];

	XFillRectangle(p->display, win, p->cleargc, 0, fontHeight(p->dfont) / 2, fontWidth(p->dfont) * 20,
	fontHeight(p->dfont));
    if ((1 << team) & warbits) {
	(void) sprintf(buf, "  %s%s", string, wars);
	XDrawImageString(p->display, win, p->dfgc, 0, fontHeight(p->dfont) / 2 + p->dfont->ascent, buf, strlen(buf));
    }
    else if ((1 << team) & hostile) {
	(void) sprintf(buf, "  %s%s", string, hostiles);
	XDrawImageString(p->display, win, p->dfgc, 0, fontHeight(p->dfont) / 2 + p->dfont->ascent, buf, strlen(buf));
    }
    else {
	(void) sprintf(buf, "  %s%s", string, peaces);
	XDrawImageString(p->display, win, p->dfgc, 0, fontHeight(p->dfont) / 2 + p->dfont->ascent, buf, strlen(buf));
    }
}

waraction(p, data)
register struct player	*p;
XKeyEvent *data;
{
    int changes;

    if (data->window == p->warf) {
	if (p->p_ship->s_swar & (1 << FED)) {
	    warning(p, "You are already at war with the Federation");
	    XBell(p->display, 0);
	}
	else {
	    p->p_ship->s_newhostile ^= (1 << FED);
	}
    }
    if (data->window == p->warr) {
	if (p->p_ship->s_swar & (1 << ROM)) {
	    warning(p, "You are already at war with the Romulans");
	    XBell(p->display, 0);
	}
	else {
	    p->p_ship->s_newhostile ^= (1 << ROM);
	}
    }
    if (data->window == p->wark) {
	if (p->p_ship->s_swar & (1 << KLI)) {
	    warning(p, "You are already at war with the Klingons");
	    XBell(p->display, 0);
	}
	else {
	    p->p_ship->s_newhostile ^= (1 << KLI);
	}
    }
    if (data->window == p->waro) {
	if (p->p_ship->s_swar & (1 << ORI)) {
	    warning(p, "You are already at war with the Orions");
	    XBell(p->display, 0);
	}
	else {
	    p->p_ship->s_newhostile ^= (1 << ORI);
	}
    }
    warrefresh(p);

    if (data->window == p->wargo) {
	changes = p->p_ship->s_hostile ^ p->p_ship->s_newhostile;
	if (changes & (1 << FED)) {
	    sendwarn(p, "Federation", p->p_ship->s_newhostile & (1 << FED), FED);
	}
	if (changes & (1 << ROM)) {
	    sendwarn(p, "Romulans", p->p_ship->s_newhostile & (1 << ROM), ROM);
	}
	if (changes & (1 << KLI)) {
	    sendwarn(p, "Klingons", p->p_ship->s_newhostile & (1 << KLI), KLI);
	}
	if (changes & (1 << ORI)) {
	    sendwarn(p, "Orions", p->p_ship->s_newhostile & (1 << ORI), ORI);
	}
	p->p_ship->s_hostile = p->p_ship->s_newhostile;
	if (p->p_ship->s_reprogram) {
	    p->p_ship->s_delay = p->p_ship->s_updates + isGod(p) ? 0 : (UPS * 10);
	    p->p_ship->s_flags |= SFWAR;	/* stop copilots, mostly */
	    warning(p, "Pausing ten seconds to re-program battle computers.");
	}
	XUnmapWindow(p->display, p->war);
	return;
    }

    if (data->window == p->warno) {
	XUnmapWindow(p->display, p->war);
	return;
    }
}

sendwarn(p, string, atwar, team)
register struct player	*p;
char *string;
int atwar;
int team;
{
    char buf[BUFSIZ];
    char addrbuf[10];

    if (atwar) {
	(void) sprintf(buf, "%s (%c%x) declaring war on the %s",
	    p->p_name,
	    teamlet[p->p_ship->s_team],
	    p->p_ship->s_no,
	    string);
	p->p_ship->s_reprogram = 1;
    }
    else {
	(void) sprintf(buf, "%s (%c%x) declaring peace with the %s",
	    p->p_name,
	    teamlet[p->p_ship->s_team],
	    p->p_ship->s_no,
	    string);
    }

    (void) sprintf(addrbuf, " %c%x->%-3s",
	teamlet[p->p_ship->s_team],
	p->p_ship->s_no,
	teamshort[team]);
    pmessage(buf, team, MTEAM, addrbuf);
}
