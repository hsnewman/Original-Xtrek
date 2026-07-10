static char sccsid[] = "@(#)interface.c	3.1";
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

/* This file will include all the interfaces between the input routines
    and the daemon.  They should be useful for writing robots and the
    like */

#include <X11/Xlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "defs.h"
#include "data.h"

set_speed(p, speed)
register struct player	*p;
int speed;
{
    p->p_ship->s_desspeed = speed;
    p->p_ship->s_flags &= ~(SFREPAIR | SFBOMB | SFORBIT | SFBEAMUP | SFBEAMDOWN);
}

set_course(p, dir)
register struct player	*p;
unsigned char dir;
{
    p->p_ship->s_desdir = dir;
    p->p_ship->s_flags &= ~(SFBOMB | SFORBIT | SFBEAMUP | SFBEAMDOWN);
}

shield_up(p)
register struct player	*p;
{
    p->p_ship->s_flags |= SFSHIELD;
    p->p_ship->s_flags &= ~(SFBOMB | SFREPAIR | SFBEAMUP | SFBEAMDOWN);
}

shield_down(p)
register struct player	*p;
{
    p->p_ship->s_flags &= ~SFSHIELD;
}

shield_tog(p)
register struct player	*p;
{
    p->p_ship->s_flags ^= SFSHIELD;
    p->p_ship->s_flags &= ~(SFBOMB | SFREPAIR | SFBEAMUP | SFBEAMDOWN);
}

bomb_planet(p)
register struct player	*p;
{
    if (!(p->p_ship->s_flags & SFORBIT)) {
	warning(p, "Must be orbiting to bomb");
	return;
    }
    p->p_ship->s_flags |= SFBOMB;
    p->p_ship->s_flags &= ~(SFSHIELD | SFREPAIR | SFBEAMUP | SFBEAMDOWN);
}

beam_up(p)
register struct player	*p;
{
    if (!(p->p_ship->s_flags & SFORBIT)) {
	warning(p, "Must be orbiting to beam up.");
	return;
    }
    if (!isGod(p) && p->p_ship->s_team != planets[p->p_ship->s_planet].pl_owner) {
	warning(p, "Those aren't our armies.");
	return;
    }
    p->p_ship->s_flags |= SFBEAMUP;
    p->p_ship->s_flags &= ~(SFSHIELD | SFREPAIR | SFBOMB | SFBEAMDOWN);
}

beam_down(p)
register struct player	*p;
{
    if (!(p->p_ship->s_flags & SFORBIT)) {
	warning(p, "Must be orbiting to beam down.");
	return;
    }
    p->p_ship->s_flags |= SFBEAMDOWN;
    p->p_ship->s_flags &= ~(SFSHIELD | SFREPAIR | SFBOMB | SFBEAMUP);
}

repair(p)
register struct player	*p;
{
    p->p_ship->s_desspeed = 0;
    p->p_ship->s_flags |= SFREPAIR;
    p->p_ship->s_flags &= ~(SFSHIELD | SFBOMB | SFBEAMUP | SFBEAMDOWN);
}

repair_off(p)
register struct player	*p;
{
    p->p_ship->s_flags &= ~SFREPAIR;
}

repeat_message(p)
register struct player	*p;
{
    if (++(p->p_lastm) == MAXMESSAGE) ;
	p->p_lastm = 0;
}

cloak(p)
register struct player	*p;
{
    p->p_ship->s_flags ^= SFCLOAK;
}

cloak_on(p)
register struct player	*p;
{
    p->p_ship->s_flags |= SFCLOAK;
}

cloak_off(p)
register struct player	*p;
{
    p->p_ship->s_flags &= ~SFCLOAK;
}

tow_off(p)
register struct player	*p;
{
	p->p_ship->s_flags &= ~SFTOWING;
	if (p->p_ship->s_towing >= MAXPLAYER) {
		warning(p, "Towing planets is not implemented.");
		return;
	}
	warning(p, "Tractor beam off.");
	ships[p->p_ship->s_towing].s_flags &= ~SFTOWED;
}

void
tow_on(p, ob)
register struct player	*p;
register struct obtype	*ob;
{
	register struct ship	*towee;
	char obuf[132];
	int	dist;
	extern double hypot();

	/* Must check that this is legal to do. */
	if (!(ob->o_type & PLAYERTYPE)) {
		warning(p, "Towing planets is not implemented.");
		return;
	}

	towee = &ships[ob->o_num];
	dist = hypot((double) (p->p_ship->s_x - towee->s_x),
		     (double) (p->p_ship->s_y - towee->s_y));

	/* Is ok to capture? */
	if (!isGod(p) && dist > MAX_TB_RANGE) {
		sprintf(obuf, "%c%x is out of range of the tractor beams",
			teamlet[towee->s_team], towee->s_no);
		warning(p, obuf);
		return;
	}

	if (!isGod(p) && ((random() % MAX_TB_RANGE) / (p->p_ship->s_speed + towee->s_speed + 1)) > dist) {
		sprintf(obuf, "The tractor beams failed to capture %c%x",
			teamlet[towee->s_team], towee->s_no);
		warning(p, obuf);
		return;
	}

	p->p_ship->s_towing = ob->o_num;
	p->p_ship->s_flags |= SFTOWING;


	towee->s_flags |= SFTOWED;
	towee->s_flags &= ~SFORBIT;
	towee->s_towed = p->p_ship->s_no;

	sprintf(obuf, "You have %c%x in your tractor beam",
		teamlet[towee->s_team], towee->s_no);
	warning(p, obuf);
}
