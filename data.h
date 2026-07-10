/* static char sccsid[] = "@(#)data.h	3.1"; */
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

extern int	xtrek_socket;

extern char	DIR[128];

extern struct player players[MAXPLAYER];
extern struct ship ships[MAXPLAYER];
extern struct torp torps[MAXPLAYER * MAXTORP];
extern struct planet planets[MAXPLANETS];
extern struct planet pdata[MAXPLANETS];
extern struct phaser phasers[MAXPLAYER];
extern struct message messages[MAXMESSAGE];
extern struct mctl mctl[MAXMESSAGE];
extern struct universe universe;

extern int remap[];
extern int udcounter;
extern int tcount[MAXTEAM + 1];

extern long	isin[], icos[];

extern char teamlet[];
extern char *teamshort[];
extern char *teamlong[];
