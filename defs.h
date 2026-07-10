/* static char sccsid[] = "@(#)defs.h	3.1"; */
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

#include "xtrekfont.h"

#define	PROGRAM_NAME	"xtrek"

#define	DEF_TCP_PORT	5701	/* The default TCP port if not in services */

#define MAXPLAYER	16
#define MAXPLANETS	40
#define	MAXPLANETTYPES	8
#define MAXTORP		12	/* per player */

/* These are configuration definitions */

/* Timing */
#define UPDATE 200000			/* Update time is 200000 us */
#define UPS (1000000/UPDATE)
#define WARP1 (200 / UPS) 		/* warp 1 moves 200 spaces per sec */
#define DEATHTIME (6 * UPS) 		/* Player is dead for 6 seconds */
#define PFIRETIME (1 * UPS) 		/* Phaser fires for 1 second */
#define TFIREMIN (3 * UPS) 		/* Torp lives at least 3 sec */
#define TFIREVAR (6 * UPS)		/* Torp may live up to 6 more sec */
#define TEXPTIME (1 * UPS)		/* Torp explodes for 1 sec */
#define PWEAPLOCKMIN (10 * UPS)		/* Weapons lock for at least 10 sec */
#define PWEAPLOCKVAR (15 * UPS)		/* Weapons may lock for 15 sec more */
#define PENGLOCKMIN (10 * UPS) 		/* Engines lock for at least 10 sec */
#define PENGLOCKVAR (15 * UPS) 		/* Engines may lock for 15 sec more */
#define PSELFDESTTIME (10 * UPS)	/* Self destruct in 10 sec */
#define RGIVEUPTIME (60 * UPS)		/* Robot gives up in 60 sec */
#define ORBSPEED 2	/* This is the fastest a person can go into orbit */
#define PEXPTIME (10)	/* Player explodes for 10 frames (2 sec) */
#define AUTOQUIT 60	/* auto logout in 60 secs */

/* Space */
#define GWIDTH 100000   /* galaxy is 100000 spaces on a side */
#define SCALE 40	/* Window will be one pixel for this many spaces */
#define EXPDIST 400	/* At this range a torp will explode */
#define DETDIST 2500	/* At this range a player can detonate a torp */
#define TDAMDIST 1500   /* At this range a torp does damage when it explodes */
#define ORBDIST 900	/* At this range a player can orbit a planet */
#define PFIREDIST 1500	/* At this range a planet will shoot at a player */
#define	DEF_XWINSIZE 500	/* Default X coord window size */
#define	DEF_YWINSIZE 500	/* Default Y coord window size */
#define	MAX_TB_RANGE 10000	/* Tractor beams are ineffective beyond this range */

/* These are memory sections */
#define PLAYER 1
#define MAXMESSAGE 50

#define rosette(x)	((((x) + 256/VIEWS/2) / (256/VIEWS)) % VIEWS)

/* These are the teams */
/* Note that I used bit types for these mostly for messages and
   war status.  This was probably a mistake.  It meant that Ed
   had to add the 'remap' area to map these (which are used throughout
   the code as the proper team variable) into a nice four team deep
   array for his color stuff.  Oh well.
*/
#define FED		1
#define ROM		2
#define KLI		3
#define ORI		4
#define NUMTEAM		4	/* Four "real" teams */
#define	SELFRULED	5
#define	NOTEAM		6
#define	GOD		7
#define MAXTEAM		GOD
/*
** These are random configuration variables
*/
#define VICTORY 30	/* Number of planets needed to conquer the galaxy */
#define WARNTIME 30	/* Number of updates to have a warning on the screen */
#define MESSTIME 30	/* Number of updates to have a message on the screen */
#define	DEF_MESSAGESIZE	20	/* Height of message window */
#define	DEF_BORDER	4	/* Size of window borders */

#define TARG_PLAYER	0x1	/* Flags for gettarget */
#define TARG_PLANET	0x2
#define TARG_CLOAK	0x4	/* Include cloaked ships in search */
#define	TARG_MYSELF	0x8	/* Allow myself */

/* Data files to make the game play across daemon restarts. */

#define PLFILE		"planets"
#define SCOREFILE	"scores"
#define MOTD		"motd"

#include "struct.h"

/* Other stuff that Ed added */

#define ABS(a)			/* abs(a) */ (((a) < 0) ? -(a) : (a))

#define myTorp(t)		(p->p_ship->s_no == (t)->t_owner)
#define friendlyTorp(t)		((!((1 << p->p_ship->s_team) & (t)->t_war)) || (myTorp(t)))
#define myPhaser(x)		(&phasers[p->p_ship->s_no] == (x))
#define friendlyPhaser(x)	(p->p_ship->s_team == players[(x) - phasers].p_ship->s_team)
#define myPlayer(x)		(p == (x))
#define myPlanet(x)		(p->p_ship->s_team == (x)->pl_owner)
#define friendlyPlayer(x)	((!((1<<p->p_ship->s_team) & \
				    ((x)->p_ship->s_swar | (x)->p_ship->s_hostile))) && \
				    (!((1<<(x)->p_ship->s_team) & \
				    (p->p_ship->s_swar | p->p_ship->s_hostile))))
#define isAlive(x)		((x)->p_status == PALIVE)
#define	isGod(x)		((x)->p_ship->s_team == GOD)
#define friendlyPlanet(x)	((x)->pl_info & (1 << p->p_ship->s_team) && \
			!((1 << (x)->pl_owner) & (p->p_ship->s_swar | p->p_ship->s_hostile)))

#define torpColor(t)		\
	(myTorp(t) ? p->myColor : p->shipCol[players[(t)->t_owner].p_ship->s_team])
#define phaserColor(x)		\
	(myPhaser(x) ? p->myColor : p->shipCol[players[(x) - phasers].p_ship->s_team])
#define playerColor(x)		\
	(myPlayer(x) ? p->myColor : p->shipCol[(x)->p_ship->s_team])
#define planetColor(x)		\
	((x)->pl_type == MOON ? p->moonColor : \
	((x)->pl_type == SUN ? p->sunColor : \
	(((x)->pl_info & (1 << p->p_ship->s_team)) ? p->shipCol[(x)->pl_owner] : p->unColor)))
#define mplanetGlyph(x)		\
	((x)->pl_type == MOON ? MMOON_GLYPH : \
	((x)->pl_type == SUN ? MSUN_GLYPH : \
	(((((x)->pl_info & (1 << p->p_ship->s_team)) ? \
	( p->mono ? (x)->pl_owner : 0 ) : 0)+MPLANET_GLYPHS))))
#define planetGlyph(x)		\
	((x)->pl_type == MOON ? MOON_GLYPH : \
	((x)->pl_type == SUN ? SUN_GLYPH : \
	(((((x)->pl_info & (1 << p->p_ship->s_team)) ? \
	( p->mono ? (x)->pl_owner : 0 ) : 0)+PLANET_GLYPHS))))

#define planetFont(x)		\
	(myPlanet(x) ? p->bfont : friendlyPlanet(x) ? p->ifont : p->dfont)
#define shipFont(x)		\
	(myPlayer(x) ? p->bfont : friendlyPlayer(x) ? p->ifont : p->dfont)

/* Macros for X11 (nurk) */
#define fontWidth(f)	((f)->max_bounds.width)
#define fontHeight(f)	((f)->max_bounds.ascent + (f)->max_bounds.descent)

#define	TRIGSCALE	13	/* trig values are multiplied by 8K */
