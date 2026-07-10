static char sccsid[] = "@(#)data.c	3.1";

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
#include "defs.h"
#include "data.h"

int	xtrek_socket;
int	udp_socket;

/* Main library directory */
char	DIR[128] = "/usr/lib/X11/xtrek";

struct player players[MAXPLAYER];
struct ship ships[MAXPLAYER];
struct torp torps[MAXPLAYER * MAXTORP];
struct planet planets[MAXPLANETS];
struct phaser phasers[MAXPLAYER];
struct message messages[MAXMESSAGE];
struct mctl mctl[MAXMESSAGE];
struct universe universe = {
	0,
	{	/* The list of people that can becomegod() */
		/* Up to MAXPLAYER of them. */
		"ddickey:fizban:0.0",
		"ddickey:aspen04:0.0",
		"ddickey:fir35:0.0",
		"ddickey:unix:0.0",
		0
	}
};

int	udcounter;
int	debug = 0;
int	tcount[MAXTEAM + 1];

extern long	isin[], icos[];		 /* Initialized in trigtab.c */

char teamlet[] = {'X', 'F', 'R', 'K', 'O', 'I', 'N', 'G'};
char *teamshort[8] = {"XXX", "FED", "ROM", "KLI", "ORI", "IND", "NOT", "GOD"};
char *teamlong[8] = {"XXX", "Federation", "Romulan Empire", "Klingon Empire", "Orion Nest", "Independent Organization", "NOT", "GOD"};
char *rnames[5] = { "", "M5", "Colossus", "Guardian", "HAL" };
