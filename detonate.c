static char sccsid[] = "@(#)detonate.c	3.1";

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
#if !defined(cray)
#include <sys/types.h>
#endif
#include "defs.h"
#include "data.h"

/* Detonate torp */

/*
** Detonating torps have become a difficult part of the game.  Players
** quickly learned that detonating their own torps when the cloud was
** around another player, caused that player to die very quickly.  I
** removed that feature because it lead to people not having to shoot
** well to collect kills.  Now when players detonate their own torps,
** the torps just vanish and become available for future firing.
** NOTE:  BOOO!  Put this back in!
** NOTE2:  It is now back in.  It should be this way.
** NOTE3:  But don't have friendly torps do damage!
*/

detmine(p)
register struct player	*p;
{
    register int i;

    if (p->p_ship->s_flags & SFWEP) {
	warning(p, "Weapons overheated");
	return;
    }
    if (p->p_ship->s_fuel < p->p_ship->s_detcost) {
	warning(p, "Not enough fuel to fire detonators.");
	return;
    }
    p->p_ship->s_fuel -= p->p_ship->s_detcost;
    p->p_ship->s_wtemp += p->p_ship->s_detcost / 25;

    for (i = 0; i < MAXTORP; i++) {
	if (torps[i + (p->p_ship->s_no * MAXTORP)].t_status == TMOVE) {
	    torps[i + (p->p_ship->s_no * MAXTORP)].t_status = TDET;
	} else if (torps[i + (p->p_ship->s_no * MAXTORP)].t_status == TSTRAIGHT) {
	    torps[i + (p->p_ship->s_no * MAXTORP)].t_status = TDET;
	}
    }
}

/*
** Here we have another flaw.  Detonating other players torps can be a
** very quick way to die.  Why?  Because you always take some damage.
** Experienced players never detonate other players' torps.  Balance is
** really hard to obtain with this type of function.  Technically, a
** player could nearly continuously detonate torps (at least faster than
** they could be fired) and never be hurt, if I allowed less damage as
** a possible result.  So here it sits.
*/

detothers(p)
register struct player	*p;
{
    register int h, i;
    int dx, dy;
    register struct torp *j;

    if (p->p_ship->s_fuel < p->p_ship->s_detcost) {
	warning(p, "Not enough fuel to fire detonators.");
	return;
    }
    if (p->p_ship->s_flags & SFWEP) {
	warning(p, "Weapons overheated");
	return;
    }
    p->p_ship->s_fuel -= p->p_ship->s_detcost;
    p->p_ship->s_wtemp += p->p_ship->s_detcost / 25;

    for (h = 0; h < MAXPLAYER; h++) {
	if ((players[h].p_status == PFREE) || (h == p->p_ship->s_no))
	    continue;
	for (i = h * MAXTORP; i < MAXTORP * (h + 1); i++) {
	    j = &torps[i];
	    if ((j->t_status == TMOVE) || (j->t_status == TSTRAIGHT)) {
		dx = j->t_x - p->p_ship->s_x;
		dy = j->t_y - p->p_ship->s_y;
		if (ABS(dx) > DETDIST || ABS(dy) > DETDIST) /* XXX */
		    continue;
		if (dx * dx + dy * dy < DETDIST * DETDIST)
		    j->t_status = TDET;
	    }
	}
    }
}
