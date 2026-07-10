static char sccsid[] = "@(#)enter.c	3.1";

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
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "data.h"

extern int	debug;

/* Enter the game */

long random();

enter(tno, disp, pno)
int tno;
char	*disp;
int pno;
{
    register struct player	*p;
    char		*pseudo, buf[80];
    register struct planet	*l;
    register int		i;

    pseudo = (char *) NULL;
    p = &players[pno];
    if (strcmp(disp, "Nowhere")) {
	pseudo = XGetDefault(p->display, PROGRAM_NAME, "name");
    }
    if (pseudo == NULL) {
	if (strcmp(disp, "Nowhere"))
		(void) strncpy(p->p_name, p->p_login, sizeof (p->p_name));
    } else
	(void) strncpy(p->p_name, pseudo, sizeof (p->p_name));
    p->p_name[12] = '\0';
    p->p_login[12] = '\0';

    p->p_no = pno;
    getstats(p);
    time(&p->p_start_time);
    p->p_umsg.m_pending = 0;
    p->p_lastm = mctl->mc_current;
    p->p_status = PALIVE;
    p->p_mapmode = 1;
    p->p_namemode = 1;
    p->p_statmode = 1;
    p->p_warntimer = -1;
    p->p_infomapped = 0;
    p->mustexit = 0;
    p->p_redrawall = 1;
    if (debug)
	fprintf(stderr, "%d p_copilot %d\n", pno, p->p_copilot);
    if (p->p_flags & PFRSHOWSTATS) {
	p->statwin = openStats(p);
    }
    if (!p->p_copilot)
	enter_ship(p, disp, tno);

    sprintf(buf, "%c%x", teamlet[p->p_ship->s_team], p->p_ship->s_no);
    strncpy(p->p_mapchars, buf, 2);
}

void
checksystems(p)
register struct player	*p;
{
	register struct planet	*l;
	register int		i;
	int			plcount[MAXTEAM];

	/* Reset */
	p->p_pick = 0;

	/* We can always enter at our own system. */
	p->p_pick |= (1 << p->p_ship->s_team);

	if (isGod(p) || p->p_ship->s_team == 0) {
		p->p_pick |= ((1 << FED) | (1 << ROM) | (1 << KLI) | (1 << ORI));
		return;
	}

	bzero(plcount, sizeof (plcount));

	for (i = 0, l = &planets[i]; i < MAXPLANETS; i++, l++) {
		if (l->pl_type == CLASSM || l->pl_type == DEAD) {
			if (l->pl_owner == pdata[i].pl_owner)
				plcount[l->pl_owner]++;
		}
	}

	if (!plcount[FED] && planets[0].pl_owner == p->p_ship->s_team)
		p->p_pick |= (1 << FED);
	if (!plcount[ROM] && planets[1].pl_owner == p->p_ship->s_team)
		p->p_pick |= (1 << ROM);
	if (!plcount[KLI] && planets[2].pl_owner == p->p_ship->s_team)
		p->p_pick |= (1 << KLI);
	if (!plcount[ORI] && planets[3].pl_owner == p->p_ship->s_team)
		p->p_pick |= (1 << ORI);
}

enter_ship(p, disp, tno)
register struct player	*p;
char			*disp;
int			tno;
{
    char		*pseudo, buf[80];
    register struct planet	*l;
    register int		i;
    char			*customship;

    customship = (char *) NULL;
    if (strcmp(disp, "Nowhere")) {
#ifdef CUSTOMSHIP
	customship = XGetDefault(p->display, PROGRAM_NAME, p->p_shipname);
#endif
    }

    p->p_ship->s_stats.st_entries++;
    p->p_ship->s_updates = udcounter;
    p->p_ship->s_status = 0;
    p->p_ship->s_explode = 0;
    p->p_ship->s_whydead = 0;
    p->p_ship->s_whodead = 0;
    p->p_ship->s_flags = SFSHIELD;
    p->p_ship->s_dir = 0;
    p->p_ship->s_desdir = 0;
    p->p_ship->s_speed = 0;
    p->p_ship->s_desspeed = 0;
    p->p_ship->s_subspeed = 0;
    if (!p->p_ship->s_team)
	    p->p_ship->s_team = tno;
    /* Find the teams home planet...using initial setup. */
    for (i = 0, l = &pdata[0]; i < MAXPLANETS; i++, l++) {
	if ((l->pl_flags & PLHOME) && (l->pl_owner == tno))
		break;
    }
    if (i >= MAXPLANETS)	/* ...then we didn't find it. */
	l = &planets[0];	/* so just use planet[0] */
    else
	l = &planets[i];	/* now switch to current data */
    p->p_ship->s_x = l->pl_x + (random() % 10000) - 5000;
    p->p_ship->s_y = l->pl_y + (random() % 10000) - 5000;
    p->p_ship->s_ntorp = 0;
    p->p_ship->s_damage = 0;
    p->p_ship->s_subdamage = 0;
    p->p_ship->s_etemp = 0;
    p->p_ship->s_etime = 0;
    p->p_ship->s_wtemp = 0;
    p->p_ship->s_wtime = 0;
    p->p_ship->s_subshield = 0;
    p->p_ship->s_armies = 0;
#ifndef CUSTOMSHIP
    customship = (char *) NULL;
#endif
    getship(p, customship);
    p->p_ship->s_swar = 0;
    p->p_ship->s_hostile = ((1 << FED)|(1 << ROM)|(1 << KLI)|(1 << ORI));
    p->p_ship->s_hostile &= ~(1 << p->p_ship->s_team);
    if (p->p_ship->s_team == GOD)
	p->p_ship->s_hostile = 0;	/* Peaceful God */
    p->p_ship->s_shield = p->p_ship->s_maxshields;
    p->p_ship->s_fuel = p->p_ship->s_maxfuel;
    p->p_ship->s_delay = 0;
    p->p_ship->s_oldalert = SFGREEN;
    p->p_ship->s_Ten = 0;
}

findslot()
{
    register int i;

    for (i = 0; i < MAXPLAYER; i++) {
	if (players[i].p_status == PFREE) {	/* We have a free slot */
	    break;
	}
    }
    if (i < MAXPLAYER) {
	    bzero(&players[i], sizeof(struct player));  /* Slight problem for copilot */
	    players[i].p_status = PSETUP;	/* possible race code */
	    players[i].p_no = i;
    }
    return(i);
}
