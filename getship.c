static char sccsid[] = "@(#)getship.c	3.1";
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

/*
 * Enhancements added by Jeff Weinstein (jeff at polyslo.calpoly.edu).
 */


#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "defs.h"
#include "data.h"

struct ship_features {
    int s_turns;
    short s_accs;
    short s_torpdamage;
    short s_phaserdamage;
    short s_phasedist;
    short s_torpspeed;
    short s_maxspeed;
    short s_repair;
    short s_maxfuel;
    short s_torpcost;
    short s_phasercost;
    short s_detcost;
    short s_warpcost;
    short s_cloakcost;
    short s_recharge;
    int s_accint;
    int s_decint;
    short s_maxarmies;

    short s_wcool;
    short s_ecool;
    short s_maxdamage;
    short s_maxshields;

    short s_dcloak;	/* if more damage than this, then cloaks are flaky */
    short s_dcloak_on;	/* chance for damaged cloaks to start working */
    short s_dcloak_off;	/* chance for damaged cloaks to stop working */
};

static struct ship_features def_ship_chars[NUMTEAM] = {
/*TURNS ACCS TORPDAM PHASDAM PHASDIST TORPSPD MAXSPD REPAIR MAXFUEL TORPCOST */
/*PHASCOST DETCOST WARPCOST CLOAKCOST RECHARGE ACCINT DECINT MAXARM */
/*WCOOL ECOOL MAXDAM MAXSHLD  DCLOAK DCLOAK_ON DCLOAK_OFF */
{ 512000, 21, 300,    2500,   60,      11,     9,    100,   10000,   0,
  0,        100,    20,      45,       13,      12,    12,   10,
  4,      6,   100,   100,	25,	25,	25 },	/* FED */
{ 256000, 21, 600,    3125,   75,       14,     7,    150,   20000,   0,
  0,         75,    25,      30,       12,      11,    15,   14,
  8,      4,   100,   125,	25,	25,	25},	/* ROM */
{ 512000, 21, 300,    2500,   60,      11,     9,    100,   10000,   0,
  0,        100,    20,      45,       13,      12,    12,   10,
  4,      6,   100,   100,	25,	25,	25},	/* KLI */
{ 1024000,21, 100,    2000,   50,      14,    11,    100,    7500,   0,
  0,        100,    15,      45,       15,      14,    14,   6,
  3,      8,   100,   100,	25,	25,	25 }	/* ORI */
};

extern int	debug;
void becomegod();

/* fill in ship characteristics */

char *
getship(p, customship)
struct player	*p;
char		*customship;
{
    struct ship *shipp;
    struct ship_features *dshipc;
    int feature[128];
    char *savecs = customship;
    double ppower, tpower;
    char	ebuf[128];

    shipp = p->p_ship;
    bzero(feature,128 * sizeof(int));
    dshipc = &def_ship_chars[p->p_ship->s_team-1];
    shipp->s_turns = dshipc->s_turns;
    shipp->s_accs = dshipc->s_accs;
    shipp->s_torpdamage = dshipc->s_torpdamage;
    shipp->s_phaserdamage = dshipc->s_phaserdamage;
    shipp->s_phasedist = dshipc->s_phasedist;
    shipp->s_torpspeed = dshipc->s_torpspeed;
    shipp->s_maxspeed = dshipc->s_maxspeed;
    shipp->s_repair = dshipc->s_repair;
    shipp->s_maxfuel = dshipc->s_maxfuel;
    shipp->s_torpcost = dshipc->s_torpcost;
    shipp->s_phasercost = dshipc->s_phasercost;
    shipp->s_detcost = dshipc->s_detcost;
    shipp->s_warpcost = dshipc->s_warpcost;
    shipp->s_cloakcost = dshipc->s_cloakcost;
    shipp->s_recharge = dshipc->s_recharge;
    shipp->s_accint = dshipc->s_accint;
    shipp->s_decint = dshipc->s_decint;
    shipp->s_maxarmies = dshipc->s_maxarmies;
    shipp->s_wcool = dshipc->s_wcool;
    shipp->s_ecool = dshipc->s_ecool;
    shipp->s_maxdamage = dshipc->s_maxdamage;
    shipp->s_maxshields = dshipc->s_maxshields;
    shipp->s_dcloak = dshipc->s_dcloak;
    shipp->s_dcloak_on = dshipc->s_dcloak_on;
    shipp->s_dcloak_off = dshipc->s_dcloak_off;

    if (customship)
	for (; *customship > 0; customship++) {
	    if (islower(*customship)) {
		    (feature[*customship])--;
	    } else if (isupper(*customship)) {
		    (feature[tolower(*customship)])++;
	    }
	}

    /*
     * The following are the configurable options:
     * a/A  Army Transport
     * c/C  Cloaking Cost
     * d/D  Heat Dissipation (weapons & engines)
     * e/E  Engine Efficiency
     * f/F  Fuel Capacity
     * g/G  Fuel Generation
     * i/I  Invisibility While Cloaked	(NOT IMPLEMENTED)
     * m/M  Maneuverablity
     * p/P  Phaser Power (affects damage & range)
     * r/R  Repair Rate
     * s/S  Shield Strength
     * t/T  Torpedo Size
     * v/V  Torpedo Speed
     * w/W  Warp Engines (affects acceleration & top speed)
     * x/X  Ship Size
     * z/Z  Change to GOD Team - SPECIAL FEATURE
     */

    if (feature['z'])
	becomegod(p);

    /* check sanity of options */
    if (!isGod(p) && ((feature['a'] + feature['c'] + feature['d'] + feature['e'] +
	 feature['f'] + feature['g'] + feature['i'] + feature['m'] +
	 feature['p'] + feature['r'] + feature['s'] + feature['t'] +
	 feature['v'] + feature['w'] + feature['x']) > 0))
    {
	sprintf(ebuf,"ship has too many additions: %s", savecs);
	warning(p, ebuf);
        bzero(feature,128 * sizeof(int));
    }
#define rangecheck(plyr,min,value,max) \
	if (!isGod((plyr)) && (feature[(value)] < (min))) \
	{ \
	    sprintf(ebuf,"ship out of range:%c < %d",(value),(min)); \
	    warning((plyr), ebuf); \
            bzero(feature,128 * sizeof(int)); \
	} \
	if (!isGod((plyr)) && (feature[(value)] > (max))) \
	{ \
	    sprintf(ebuf,"ship out of range:%c > %d",(value),(max)); \
	    warning((plyr), ebuf); \
            bzero(feature,128 * sizeof(int)); \
	}

    /* Check all ranges before making any modification. */
    rangecheck(p,-2,'a',1000);
    rangecheck(p,0,'c',3);
    rangecheck(p,-5,'d',1000);
    rangecheck(p,-4,'e',16);
    rangecheck(p,-3,'f',9);
    rangecheck(p,-5,'g',1000);
    rangecheck(p,-2,'i',1000);
    rangecheck(p,-5,'m',1000);
    rangecheck(p,-4,'p',1000);
    rangecheck(p,-2,'r',1000);
    rangecheck(p,-4,'s',1000);
    rangecheck(p,-2,'t',1000);
    rangecheck(p,-4,'v',1000);
    rangecheck(p,-5,'w',1000);
    rangecheck(p,-5,'x',1000);

    shipp->s_maxarmies += 4 * feature['a'];
    if (shipp->s_maxarmies < 0)
	shipp->s_maxarmies = 0;

    shipp->s_cloakcost -= 10 * feature['c'];
    if (shipp->s_cloakcost < 0)
	shipp->s_cloakcost = 0;

    shipp->s_ecool += feature['d'];
    shipp->s_wcool += feature['d'];
    if (shipp->s_ecool < 0)
	shipp->s_ecool = 0;
    if (shipp->s_wcool < 0)
	shipp->s_wcool = 0;

    shipp->s_warpcost /= (5 + feature['e']);

    /*
     * this limit is artificial, an artifact of the status
     * display code, which requires fuel to be a short. YUK!!
     * I should fix it some time.
     */
    shipp->s_maxfuel += 2500 * feature['f'];

    shipp->s_recharge += 2 * feature['g'];
    if (shipp->s_recharge < 0)
	shipp->s_recharge = 0;

    shipp->s_accs += feature['i'] * 10;

    if (feature['m'] > 0) {
	shipp->s_turns <<= feature['m'];
    } else {
	shipp->s_turns >>= -(feature['m']);
    }

    ppower=(4 + feature['p']) * shipp->s_phaserdamage;
    shipp->s_phaserdamage = (int) sqrt(ppower);
    shipp->s_phasedist = shipp->s_phaserdamage * 60;

    shipp->s_repair += 50 * feature['r'];
    if (shipp->s_repair < 0)
	shipp->s_repair = 0;

    shipp->s_maxshields += 25 * feature['s'];
    if (shipp->s_maxshields < 0)
	shipp->s_maxshields = 0;

    tpower=(2 + feature['t']) * shipp->s_torpdamage;
    if (tpower < (double) 0.0)
      tpower = (double) 0.0;
    shipp->s_torpdamage = (int) sqrt(tpower);

    shipp->s_torpspeed += 6 * feature['v'];
    if (shipp->s_torpspeed < 0)
	shipp->s_torpspeed = 0;

    shipp->s_maxspeed += 2 * feature['w'];
    if (shipp->s_maxspeed < 1)
	shipp->s_maxspeed = 1;
    shipp->s_accint *= (1 + shipp->s_maxspeed);
    shipp->s_decint *= (1 + shipp->s_maxspeed);

    shipp->s_maxdamage += 15 * feature['x'];

    shipp->s_detcost = 100;
    shipp->s_torpcost = (shipp->s_torpspeed/2 + 4) * shipp->s_torpdamage;
    shipp->s_phasercost = 10 * shipp->s_phaserdamage;
    return((char *) NULL);
}

void
becomegod(p)
register struct player	*p;
{
	register int	i;
	register char	*cp;
	char		cbuf[256];

	sprintf(cbuf, "%s:%s", p->p_login, p->p_monitor);
	if (debug)
		printf("God Attempt by <%s>\n", cbuf);
	for (i = 0, cp = universe.god_names[0]; i < MAXPLAYER && cp && *cp;) {
		if (strcmp(cbuf, cp) == 0) {
			p->p_ship->s_team = GOD;
			return;
		}
		cp = universe.god_names[++i];
	}
}

update_weapons(p)
register struct player	*p;
{
	struct ship *shipp;
	double ppower, tpower;

	shipp = p->p_ship;
	return;

	/* 10% more phaser power for each kill */
	shipp->s_phaserdamage += (shipp->s_phaserdamage * (shipp->s_stats.st_kills / 10.0));
	ppower = shipp->s_phaserdamage * shipp->s_phaserdamage;
	shipp->s_phaserdamage = (int) sqrt(ppower);
	shipp->s_phasedist = shipp->s_phaserdamage * 60;

	/* 10% more torp power for each kill */
	shipp->s_torpdamage += (shipp->s_torpdamage * (shipp->s_stats.st_kills / 10.0));
	tpower = shipp->s_torpdamage * shipp->s_torpdamage;
	if (tpower < (double) 0.0)
		tpower = (double) 0.0;
	shipp->s_torpdamage = (int) sqrt(tpower);
}
