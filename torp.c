static char sccsid[] = "@(#)torp.c	3.1";
#ifndef lint
static char *rcsid_torp_c = "$Header: /uraid2/riedl/src/xceltrek/RCS/torp.c,v 1.1 88/04/18 16:10:50 riedl Exp Locker: riedl $";
#endif	lint
/* Copyright (c) 1986 	Chris Guthrie */

#include <X11/Xlib.h>
#include <stdio.h>
#if !defined(cray)
#include <sys/types.h>
#endif
#include "defs.h"
#include "data.h"
extern long isin[], icos[];

/* Launch torp */

ntorp(p, course, type)
register struct player	*p;
unsigned char course;
int type;
{
    register int i, x, y;
    register struct torp *k;
    char buf[50];
    double sqrt();

    if (p->p_ship->s_ntorp >= MAXTORP) {
        sprintf(buf, "Torps limited to %d", MAXTORP);
	warning(p, buf);
	return;
    }
    if (p->p_ship->s_fuel < p->p_ship->s_torpcost) {
	warning(p, "Not enough fuel for torp");
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
    p->p_ship->s_ntorp++;
    p->p_ship->s_fuel -= p->p_ship->s_torpcost;
    p->p_ship->s_wtemp += p->p_ship->s_torpcost / 10;
    for (i = MAXTORP, k = &torps[p->p_ship->s_no * MAXTORP]; i > 0; i--, k++) {
        if (k->t_status == TFREE)
	    break;
    }
    if (i == 0) {
      fprintf(stderr, "ntorp: sanity check failed - no torp to fire\n");
      p->p_ship->s_ntorp--;
      return;
    }
    k->t_no = MAXTORP - i;
    k->t_status = type;
    k->t_owner = p->p_ship->s_no;
    k->t_team = p->p_ship->s_team;
    k->t_x = p->p_ship->s_x;
    k->t_y = p->p_ship->s_y;
    k->t_dir = course;
    k->t_damage = p->p_ship->s_torpdamage;
    k->t_speed = p->p_ship->s_torpspeed;
    k->t_war = p->p_ship->s_hostile | p->p_ship->s_swar;
    k->t_fuse = (random() % TFIREVAR) + TFIREMIN;
    p->p_ship->s_stats.st_torps++;
}
