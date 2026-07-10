static char sccsid[] = "@(#)phaser.c	3.1";

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

phaser(p, course)
register struct player	*p;
unsigned char course;
{
    register int i;
    register struct player *j, *target;
    register struct phaser *mine;
    unsigned char dir, iatan2();
    int range, trange;
    char buf[80];

    mine = &phasers[p->p_ship->s_no];

    if (p->p_ship->s_phaserdamage <= 0) {
	warning(p, "You don't have phasers");
	return;
    }
    if (mine->ph_status != PHFREE) {
	warning(p, "Phasers have not recharged");
	return;
    }
    if (p->p_ship->s_fuel < p->p_ship->s_phasercost) {
	warning(p, "Not enough fuel for phaser");
	return;
    }
    if (!isGod(p) && (p->p_ship->s_flags & SFREPAIR)) {
	warning(p, "Can't fire while repairing");
	return;
    }
    if (!isGod(p) && (p->p_ship->s_flags & SFWEP)) {
	warning(p, "Weapons overheated");
	return;
    }
    if (!isGod(p) && (p->p_ship->s_flags & SFCLOAK)) {
	warning(p, "Cannot fire while cloaked");
	return;
    }

    p->p_ship->s_fuel -= p->p_ship->s_phasercost;
    p->p_ship->s_wtemp += p->p_ship->s_phasercost / 10;
    target = (struct player *) 0;
    mine->ph_dir = course;
    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if ((j->p_status != PALIVE) || (j == p))
	    continue;
	if (isGod(j)) continue;
	if ((!((j->p_ship->s_swar | j->p_ship->s_hostile) & (1 << p->p_ship->s_team))) &&
	    (!((p->p_ship->s_swar | p->p_ship->s_hostile) & (1 << j->p_ship->s_team))))
		continue;
	dir = iatan2(j->p_ship->s_x - p->p_ship->s_x, p->p_ship->s_y - j->p_ship->s_y);
	if (angdist(dir, course) < 5) {
	    trange = (int) hypot((double) (j->p_ship->s_x - p->p_ship->s_x),
		(double) (j->p_ship->s_y - p->p_ship->s_y));
	    if (target == 0) {
		target = j;
		range = trange;
	    }
	    else if (range > trange) {
		target = j;
		range = trange;
	    }
	}
    }
    if ((target == 0) || (range > p->p_ship->s_phasedist)) {
	mine->ph_fuse = 10;
	mine->ph_status = PHMISS;
	warning(p, "Phaser missed!!!");
    }
    else {
	mine->ph_fuse = 10;
	mine->ph_target = target->p_ship->s_no;
	mine->ph_damage = (p->p_ship->s_phasedist - range) * p->p_ship->s_phaserdamage / p->p_ship->s_phasedist;
	mine->ph_status = PHHIT;
	(void) sprintf(buf, "Phaser hit %s for %d points",
	    target->p_name,
	    mine->ph_damage);
	warning(p, buf);
    }
    p->p_ship->s_stats.st_phasers++;
}
