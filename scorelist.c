static char sccsid[] = "@(#)scorelist.c	3.1";
/* Copyright (c) 1986 	Chris Guthrie */


#include <X11/Xlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "defs.h"
#include "data.h"
#include "bitmaps.h"

struct score {
	int	s_no;
	char	*p_name;
	double	st_kills;
	double	st_maxkills;
	int	st_losses;
	double	skill;
	int	st_torps;
	int	st_phasers;
	int	st_planets;
	int	st_armsbomb;
} player_scores[MAXPLAYER];

score_cmp(s1, s2)
register struct score	*s1, *s2;
{
	if (s1->skill < s2->skill)
		return(1);
	if (s1->skill > s2->skill)
		return(-1);
	if (s1->st_kills < s2->st_kills)
		return(1);
	if (s1->st_kills > s2->st_kills)
		return(-1);
	if (s1->st_maxkills < s2->st_maxkills)
		return(1);
	if (s1->st_maxkills > s2->st_maxkills)
		return(-1);
	return(0);
}

scorelist(p, fid)
register struct player	*p;
int			fid;
{
    register int i, t;
    register int k = 0;
    char buf[BUFSIZ];
    register struct player *j;
    struct player team;
    int active_team;
    int	ps_index;
    struct score	*psp;

    strcpy(buf, " # Name              Kills MaxK  Killed Skill    Torps Phasers Planets Bombed");
    if (p) {
	    p->p_infomapped = 1;
	    p->infow = XCreateWindow(p->display, p->mapw, 10, 10, 77 * fontWidth(p->dfont),
		(MAXPLAYER + NUMTEAM + 4) * fontHeight(p->dfont), 2, DefaultDepth(p->display, p->screen), InputOutput,
		(Visual *) CopyFromParent, 0L, (XSetWindowAttributes *) 0);
	/*	(MAXPLAYER + NUMTEAM + 4) * fontHeight(dfont), 2, p->foreTile, p->backTile);*/
	    XSetWindowBackground(p->display, p->infow, p->backColor);
	    XMapWindow(p->display, p->infow);
	    XDrawImageString(p->display, p->infow, p->dfgc, 0, fontHeight(p->dfont), buf,
	       strlen(buf));
	    k = 2;
    } else {
	write(fid, buf, strlen(buf));
	write(fid, "\n", 1);
    }

    ps_index = 0;
    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if (j->p_status == PFREE)
	    continue;
	psp = &player_scores[ps_index++];
	psp->s_no = j->p_ship->s_no;
	psp->p_name = j->p_name;
	psp->st_kills = j->p_stats.st_kills;
	psp->st_maxkills = j->p_stats.st_maxkills;
	psp->st_losses = j->p_stats.st_losses;
	psp->skill = (j->p_stats.st_losses == 0) ?
	    (j->p_stats.st_kills * 2) : j->p_stats.st_kills / j->p_stats.st_losses;
	psp->st_torps = j->p_stats.st_torps;
	psp->st_phasers = j->p_stats.st_phasers;
	psp->st_planets = j->p_stats.st_planets;
	psp->st_armsbomb = j->p_stats.st_armsbomb;
    }
    qsort(player_scores, ps_index, sizeof (struct score), score_cmp);

    for (i = 0, psp = &player_scores[0]; i < ps_index; i++, psp++) {
	sprintf(buf, " %1x %-16.16s %6.2f %5.2f %6d %5.2f %7d %7d %7d %6d",
		psp->s_no,
		psp->p_name,
		psp->st_kills,
		psp->st_maxkills,
		psp->st_losses,
		psp->skill,
		psp->st_torps,
		psp->st_phasers,
		psp->st_planets,
		psp->st_armsbomb
		);
	if (!p) {
		write(fid, buf, strlen(buf));
		write(fid, "\n", 1);
	} else {
		XDrawImageString(p->display, p->infow, p->dfgc, 0, fontHeight(p->dfont) * k++, buf, strlen(buf));
	}
    }

    k++;	/* leave a blank line between players and teams */
    if (!p)
	write(fid, "\n", 1);

    for (t = 0; t < NUMTEAM; t++) {
	team.p_no = t;
	if (t == FED)
		strcpy(team.p_name, "Federation");
	else if (t == ROM)
		strcpy(team.p_name, "Romulan");
	else if (t == ORI)
		strcpy(team.p_name, "Orion");
	else if (t == KLI)
		strcpy(team.p_name, "Klingon");
	team.p_stats.st_kills = 0;
	team.p_stats.st_maxkills = 0;
	team.p_stats.st_losses = 0;
	team.p_stats.st_torps = 0;
	team.p_stats.st_phasers = 0;
	team.p_stats.st_planets = 0;
	team.p_stats.st_armsbomb = 0;
	active_team = 0;
	for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	    if (j->p_status == PFREE || j->p_ship->s_team != t)
		continue;
	    if (j->p_copilot)
		continue;
	    active_team = 1;
	    team.p_stats.st_kills    += j->p_ship->s_stats.st_kills;
	    if (team.p_stats.st_maxkills < j->p_stats.st_maxkills)
	      team.p_stats.st_maxkills = j->p_stats.st_maxkills;
	    team.p_stats.st_losses   += j->p_ship->s_stats.st_losses;
	    team.p_stats.st_torps    += j->p_ship->s_stats.st_torps;
	    team.p_stats.st_phasers  += j->p_ship->s_stats.st_phasers;
	    team.p_stats.st_planets  += j->p_ship->s_stats.st_planets;
	    team.p_stats.st_armsbomb += j->p_ship->s_stats.st_armsbomb;
	}
	if (active_team) {
	  sprintf(buf, " %1x %-16.16s %6.2f %5.2f %6d %5.2f %7d %7d %7d %6d",
		team.p_no,
		team.p_name,
		team.p_stats.st_kills,
		team.p_stats.st_maxkills,
		team.p_stats.st_losses,
		(team.p_stats.st_losses == 0) ?
		    (team.p_stats.st_kills * 2) : team.p_stats.st_kills / team.p_stats.st_losses,
		team.p_stats.st_torps,
		team.p_stats.st_phasers,
		team.p_stats.st_planets,
		team.p_stats.st_armsbomb
		);
	  if (!p) {
		write(fid, buf, strlen(buf));
		write(fid, "\n", 1);
	  } else {
		XDrawImageString(p->display, p->infow, p->dfgc, 0, fontHeight(p->dfont) * k++, buf, strlen(buf));
	  }
	}
    }
}
