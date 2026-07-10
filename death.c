static char sccsid[] = "@(#)death.c	3.1";
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
#include <signal.h>
#include <setjmp.h>
#if !defined(cray)
#include <sys/types.h>
#endif

#ifdef hpux
#include <time.h>
#else hpux
#include <sys/time.h>
#endif hpux

#include "defs.h"
#include "data.h"

death(p)
register struct player	*p;
{
    char buf[256];
    register struct player	*j;

    switch (p->p_ship->s_whydead) {
	case KQUIT:
	    sprintf(buf, "You have self-destructed");
	    warning(p, buf);
	    break;
	case KTORP:
	    sprintf(buf, "You were killed by a torp from %s (%c%x) who was %d%% damaged",
		players[p->p_ship->s_whodead].p_name,
		teamlet[players[p->p_ship->s_whodead].p_ship->s_team],
		p->p_ship->s_whodead,
		players[p->p_ship->s_whodead].p_ship->s_damage);
	    warning(p, buf);
	    break;
	case KLIGHTNING:
	    sprintf(buf, "You were killed by a lightning bolt from %s (%c%x)",
		players[p->p_ship->s_whodead].p_name,
		teamlet[players[p->p_ship->s_whodead].p_ship->s_team],
		p->p_ship->s_whodead);
	    warning(p, buf);
	    break;
	case KPHASER:
	    sprintf(buf, "You were killed by a phaser shot from %s (%c%x) who was %d%% damaged",
		players[p->p_ship->s_whodead].p_name,
		teamlet[players[p->p_ship->s_whodead].p_ship->s_team],
		p->p_ship->s_whodead,
		players[p->p_ship->s_whodead].p_ship->s_damage);
	    warning(p, buf);
	    break;
	case KPLANET:
	    if (planets[p->p_ship->s_whodead].pl_type == SUN)
		    sprintf(buf, "You were killed by radiation from %s",
			planets[p->p_ship->s_whodead].pl_name);
	    else
	    sprintf(buf, "You were killed by planetary fire from %s (%c)",
		planets[p->p_ship->s_whodead].pl_name,
		teamlet[planets[p->p_ship->s_whodead].pl_owner]);
	    warning(p, buf);
	    break;
	case KSHIP:
	    sprintf(buf, "You were killed by an exploding ship formerly owned by %s (%c%x) who was %d%% damaged",
		players[p->p_ship->s_whodead].p_name,
		teamlet[players[p->p_ship->s_whodead].p_ship->s_team],
		p->p_ship->s_whodead,
		players[p->p_ship->s_whodead].p_ship->s_damage);
	    warning(p, buf);
	    break;
	case KWINNER:
	    sprintf(buf, "Galaxy has been conquered by %s (%c%x)",
		players[p->p_ship->s_whodead].p_name,
		teamlet[players[p->p_ship->s_whodead].p_ship->s_team],
		players[p->p_ship->s_whodead].p_ship->s_no);
	    warning(p, buf);
	    break;
	case KVAPOR:
	    /* The display is vapor...can't tell them anything. */
	    break;
	default:
	    sprintf(buf, "You were killed by something unknown to this game?");
	    warning(p, buf);
	    p->mustexit = 1;
	    break;
	}

    if (p->p_ship->s_whydead != KQUIT && p->p_ship->s_whydead != KWINNER)
	    p->p_ship->s_stats.st_losses++;

    for (j = &players[0]; j < &players[MAXPLAYER]; j++) {
	if (p->p_ship == j->p_ship) {
		calcstats(j);
		savestats(j);
	}
    }
    if (p->p_ship)
	    resetsstats(p);

    p->p_status = PDEAD;
    kill_copilots(p);
    p->p_ship->s_explode = DEATHTIME;

    /* If we were being towed, turn off the tower's tractor beam */
    if (p->p_ship->s_flags & SFTOWED) {
	ships[p->p_ship->s_towed].s_flags &= ~SFTOWING;
    }
    /* Turn our own tractor beam off. */
    tow_off(p);

    if (!(p->p_flags & PFROBOT) && p->p_ship->s_whydead != KVAPOR) {
	    XClearWindow(p->display, p->w);
	    if (!p->mono) {
/*ICCCM 4.1.9	    XSetWindowBorder(p->display, p->baseWin, p->gColor);*/
/*ICCCM 4.1.9	    XSetWindowBorder(p->display, p->iconWin, p->gColor);*/
	    } else {
/*ICCCM 4.1.9	    XSetWindowBorderPixmap(p->display, p->baseWin, p->gTile);*/
/*ICCCM 4.1.9	    XSetWindowBorderPixmap(p->display, p->iconWin, p->gTile);*/
	    }
    }

	if (!(p->p_flags & PFROBOT) && p->p_ship->s_whydead != KVAPOR) {
		if (ismapped(p, p->playerw))
			XUnmapWindow(p->display, p->playerw);
		if (ismapped(p, p->planetw))
			XUnmapWindow(p->display, p->planetw);
		if (p->p_infomapped)
			destroyInfo(p);
		if (ismapped(p, p->war))
			XUnmapWindow(p->display, p->war);
		if (p->p_flags & PFSHOWSTATS)
			closeStats(p, p->statwin);
		/* p->p_status = POUTFIT; */
	}
}

kill_copilots(p)
register struct player	*p;
{
	register int		i;
	register struct player	*co;

	for (i = 0, co = &players[0]; i < MAXPLAYER; i++, co++) {
		if (co->p_status == PALIVE && p->p_ship == co->p_ship && co->p_copilot) {
			co->p_status = PDEAD;
		}
	}
}
