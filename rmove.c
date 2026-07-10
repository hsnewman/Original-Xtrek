static char sccsid[] = "@(#)rmove.c	3.1";
#ifndef lint
static char *rcsid_rmove_c = "$Header: /uraid2/riedl/src/xceltrek/RCS/rmove.c,v 1.1 88/04/18 16:10:39 riedl Exp Locker: riedl $";
#endif	lint
/* Copyright (c) 1986 	Chris Guthrie */

#include <X11/Xlib.h>
#include <stdio.h>
#include "defs.h"
#include "data.h"

#define	SIZEOF(s)		(sizeof (s) / sizeof (*(s)))
#define AVOID_TIME		4
#define	AVOID_CLICKS		200

#define STAY	0x1
#define RUN	0x2
#define ATTACK	0x3
#define REPAIR	0x4
#define AVOID	0x5

#define NORMALIZE(d) 		((((d) % 256) + 256) % 256)

#define	E_TSHOT		0x02
#define	E_PSHOT		0x04
#define	E_TEAMRED	0x08
#define	E_TEAMYELLOW	0x10
#define	E_PLANETPROT	0x20

struct Enemy {
	int	e_info;
	int	e_dist;
	unsigned char	e_course;	/* course to enemy */
	unsigned char	e_pcourse;	/* course to "protected" team member */
	unsigned char	e_tcourse;	/* torpedo intercept course to enemy */
	int	e_planetprot;
	unsigned int	e_flags;
};
#define	NO_PLAYERS	(-1)

extern int debug;
extern int playerchange;
extern int nplayers;
extern int nrobots;

extern long isin[], icos[];
extern unsigned char iatan2();

double hypot();
extern unsigned char newcourse();

rmove(p)
struct player	*p;
{
    register int i;
    register int burst;
    register int numHits, tDir;
    int		avDir;
    extern struct Enemy *get_nearest();
    struct Enemy *enemy_buf;
    struct player *enemy;
    int		no_cloak;
    static int	avoid[2] = { -32, 32 };

    if (p->p_ship->s_status == EXPLODE) {
	/* Don't bother...besides, it can't! */
	/* There would be no ship to do anything with, just a few */
	/* explosion glyphs lying around on people's screens. */
	return;
    }
    /* Find an enemy */
    enemy_buf = get_nearest(p);
    if (enemy_buf == (struct Enemy *) NO_PLAYERS) {
	/* No more enemies */
	if (do_repair(p)) {
	    return;
	}
	p->p_ship->s_flags &= ~SFPLANETPROT;
	p->p_ship->s_planetprot = 0;
	go_home(p);
	if (debug > 1)
	    fprintf(stderr, "%d) No players in game.\n", p->p_ship->s_no);
	return;
    } else if (enemy_buf == 0) {	 /* no one hostile */
	if (debug > 1)
	    fprintf(stderr, "%d) No hostile players in game.\n", p->p_ship->s_no);
	if (do_repair(p)) {
	    return;
	}
	if (p->p_ship->s_flags & SFPLANETPROT) {
		go_planet(p, &planets[p->p_ship->s_planetprot], p->p_ship->s_maxspeed / 2);
	} else {
		go_home(p);
	}
	return;
    } else {
	enemy = &players[enemy_buf->e_info];
	if (enemy_buf->e_flags & (E_TEAMYELLOW|E_TEAMRED)) {
		/* Someone to kill or protect */
		p->p_ship->s_flags &= ~SFPLANETPROT;
		p->p_ship->s_planetprot = 0;
		if (debug > 1)
		    fprintf(stderr, "%d) noticed %d, dist %d\n", p->p_ship->s_no, enemy->p_ship->s_no, enemy_buf->e_dist);
	} else if (enemy_buf->e_flags & E_PLANETPROT) {
		/* A Planet to protect */
		if (debug > 1)
		    fprintf(stderr, "%d) Protecting Planet %d\n", p->p_ship->s_no, enemy_buf->e_planetprot);
		p->p_ship->s_flags |= SFPLANETPROT;
		p->p_ship->s_planetprot = enemy_buf->e_planetprot;
		go_planet(p, &planets[enemy_buf->e_planetprot], p->p_ship->s_maxspeed - 2);
	}
    }

/* Algorithm:
** We have an enemy.
** First priority: shoot at target in range.
** Second: Dodge torps.
** Third: Get away if we are damaged.
** Fourth: repair.
** Fifth: defend team members.
** Sixth: attack.
*/

/*
** If we are a practice robot, we will do all but the second.  One
** will be modified to shoot poorly and not use phasers.
**/
    /* Fire weapons!!! */
    /*
    ** get_nearest() has already determined if torpedoes and phasers
    ** will hit.  It has also determined the courses which torps and
    ** phasers should be fired.  If so we will go ahead and shoot here.
    ** We will lose repair and cloaking for the rest of this interrupt.
    ** if we fire here.
    */

    if (!(p->p_flags & (PFRHARD|PFRVHARD))) {
	/* Practice robot */
	no_cloak = 1;
	if (enemy_buf->e_flags & E_TSHOT) {
	    if (debug > 1)
		fprintf(stderr, "%d) firing torps\n", p->p_ship->s_no);
	    for (burst = 0; (burst < 2) && (p->p_ship->s_ntorp < MAXTORP); burst++) {
		/* Bad shot.. */
		ntorp(p, enemy_buf->e_tcourse + ((random() % 3) - 1), TMOVE);
	    }
	}
    } else if (!(p->p_flags & PFRVHARD)) {
	/* Hard robot */
	no_cloak = 0;
	if (enemy_buf->e_flags & E_TSHOT) {
	    if (debug > 1)
		fprintf(stderr, "%d) firing torps\n", p->p_ship->s_no);
	    for (burst = 0; (burst < 1) && (p->p_ship->s_ntorp < MAXTORP); burst++) {
		repair_off(p);
		cloak_off(p);
		ntorp(p, enemy_buf->e_tcourse, TMOVE);
		no_cloak++;
	    }
	}
	if (enemy_buf->e_flags & E_PSHOT) {
	    if (debug > 1)
		fprintf(stderr, "%d) phaser firing\n", p->p_ship->s_no);
	    no_cloak++;
	    repair_off(p);
	    cloak_off(p);
	    /* Bad shot.. */
	    phaser(p, enemy_buf->e_course + ((random() % 5) - 1));
	}
    } else {
	/* Very Hard Robot */
	no_cloak = 0;
	if (enemy_buf->e_flags & E_TSHOT) {
	    if (debug > 1)
		fprintf(stderr, "%d) firing torps\n", p->p_ship->s_no);
	    for (burst = 0; (burst < 2) && (p->p_ship->s_ntorp < MAXTORP); burst++) {
		repair_off(p);
		cloak_off(p);
		ntorp(p, enemy_buf->e_tcourse, TSTRAIGHT);
		no_cloak++;
	    }
	}
	if (enemy_buf->e_flags & E_PSHOT) {
	    if (debug > 1)
		fprintf(stderr, "%d) phaser firing\n", p->p_ship->s_no);
	    no_cloak++;
	    repair_off(p);
	    cloak_off(p);
	    phaser(p, enemy_buf->e_course);
	}
    }

    /* Avoid torps */
    /*
    ** This section of code allows robots to avoid torps.
    ** Within a specific range they will check to see if
    ** any of the 'closest' enemies torps will hit them.
    ** If so, they will evade for four updates.
    ** Evading is all they will do for this round, other than shooting.
    */

    if ((enemy_buf->e_flags & (E_TEAMRED|E_TEAMYELLOW|E_PLANETPROT)) == 0 && (p->p_flags & PFRVHARD)) {
	if (enemy->p_ship->s_ntorp < 5) {
	    if ((enemy_buf->e_dist < 15000) || (p->p_ship->s_timer > 0)) {
		numHits = projectDamage(p, enemy->p_ship->s_no, &avDir);
		if (debug > 1) {
		    fprintf(stderr, "%d hits expected from %d from dir = %d\n",
			    numHits, enemy->p_ship->s_no, avDir);
		}
		if (numHits == 0) {
		    if (--p->p_ship->s_timer > 0) {	/* we may still be avoiding */
			if (angdist(p->p_ship->s_desdir, p->p_ship->s_dir) > 64)
			    p->p_ship->s_desspeed = 3;
			else
			    p->p_ship->s_desspeed = 5;
			return;
		    }
		} else {
		    /*
		     * Actually avoid Torps
		     */
		    p->p_ship->s_timer = AVOID_TIME;
		    tDir = avDir - p->p_ship->s_dir;
		    /* put into 0->255 range */
		    tDir = NORMALIZE(tDir);
		    if (debug > 1)
			fprintf(stderr, "mydir = %d avDir = %d tDir = %d q = %d\n",
			    p->p_ship->s_dir, avDir, tDir, tDir / 64);
		    switch (tDir / 64) {
		    case 0:
		    case 1:
			    p->p_ship->s_desdir = NORMALIZE(avDir + 64);
			    break;
		    case 2:
		    case 3:
			    p->p_ship->s_desdir = NORMALIZE(avDir - 64);
			    break;
		    }
		    if (!no_cloak)
			cloak_on(p);

		    if (angdist(p->p_ship->s_desdir, p->p_ship->s_dir) > 64)
			p->p_ship->s_desspeed = 3;
		    else
			p->p_ship->s_desspeed = 5;

		    shield_up(p);
		    if (debug > 1)
			fprintf(stderr, "evading to dir = %d\n", p->p_ship->s_desdir);
		    return;
		}
	    }
	}

	/*
	** Trying another scheme.
	** Robot will keep track of the number of torps a player has
	** launched.  If they are greater than say four, the robot will
	** veer off immediately.  Seems more humanlike to me.
	*/

	else if (enemy_buf->e_dist < 15000) {
	    if (--p->p_ship->s_timer > 0) {	/* we may still be avoiding */
		if (angdist(p->p_ship->s_desdir, p->p_ship->s_dir) > 64)
		    p->p_ship->s_desspeed = 3;
		else
		    p->p_ship->s_desspeed = 5;
		return;
	    }
	    if (random() % 2) {
		p->p_ship->s_desdir = NORMALIZE(enemy_buf->e_course - 64);
		p->p_ship->s_timer = AVOID_TIME;
	    }
	    else {
		p->p_ship->s_desdir = NORMALIZE(enemy_buf->e_course + 64);
		p->p_ship->s_timer = AVOID_TIME;
	    }
	    if (angdist(p->p_ship->s_desdir, p->p_ship->s_dir) > 64)
		p->p_ship->s_desspeed = 3;
	    else
		p->p_ship->s_desspeed = 5;
	    shield_up(p);
	    return;
	}
    }

    /* Run away */
    /*
    ** The robot has taken damage.  He will now attempt to run away from
    ** the closest player.  This obviously won't do him any good if there
    ** is another player in the direction he wants to go.
    ** Note that the robot will not run away if he dodged torps, above.
    ** The robot will lower his shields in hopes of repairing some damage.
    */

    if ((enemy_buf->e_flags & (E_TEAMRED|E_TEAMYELLOW|E_PLANETPROT)) == 0 && (p->p_ship->s_damage > 0 && enemy_buf->e_dist < 13000)) {
	if (p->p_ship->s_etemp > 900)		/* 90% of 1000 */
	    p->p_ship->s_desspeed = 5;
	else
	    p->p_ship->s_desspeed = 6;
	if (!no_cloak)
	    cloak_on(p);
	repair_off(p);
	shield_down(p);
	p->p_ship->s_desdir = enemy_buf->e_course - 128;
	if (debug > 1)
	    fprintf(stderr, "%d(%d)(%d/%d) running from %c%d %16s damage (%d/%d) dist %d\n",
		p->p_ship->s_no,
		(int) p->p_ship->s_stats.st_kills,
		p->p_ship->s_damage,
		p->p_ship->s_shield,
		teamlet[enemy->p_ship->s_team],
		enemy->p_ship->s_no,
		enemy->p_login,
		enemy->p_ship->s_damage,
		enemy->p_ship->s_shield,
		enemy_buf->e_dist);
	return;
    }

    /* Repair if necessary (we are safe) */
    /*
    ** The robot is safely away from players.  It can now repair in peace.
    ** It will try to do so now.
    */

    if (do_repair(p)) {
	return;
    }

    /* Defend. */
    if (enemy_buf->e_flags & (E_TEAMRED|E_TEAMYELLOW)) {
	if (enemy_buf->e_flags & E_TEAMRED) {
		set_course(p, enemy_buf->e_pcourse);
		cloak_off(p);
		set_speed(p, p->p_ship->s_maxspeed);
		if (debug > 1)
		    fprintf(stderr, "%d) Code RED, defending %s (%c%d)\n",
			p->p_ship->s_no, enemy->p_name,
			teamlet[enemy->p_ship->s_team], enemy->p_ship->s_no);
	} else if (enemy_buf->e_flags & E_TEAMYELLOW) {
		set_course(p, enemy_buf->e_pcourse);
		cloak_off(p);
		set_speed(p, p->p_ship->s_maxspeed - 2);
		if (debug > 1)
		    fprintf(stderr, "%d) Code YELLOW, defending %s (%c%d)\n",
			p->p_ship->s_no, enemy->p_name,
			teamlet[enemy->p_ship->s_team], enemy->p_ship->s_no);
	}
    }

    /* Attack. */
    /*
    ** The robot has nothing to do.  It will check and see if the nearest
    ** enemy fits any of its criterion for attack.  If it does, the robot
    ** will speed in and deliver a punishing blow.  (Well, maybe)
    */

    if ((enemy_buf->e_dist < 25000) || (p->p_flags & PFRHOSTILE)) {
	if ((!no_cloak) && (enemy_buf->e_dist < 10000))
	    cloak_on(p);
	p->p_ship->s_flags &= ~SFPLANETPROT;
	p->p_ship->s_planetprot = 0;
	shield_up(p);
	if (debug > 1)
	    fprintf(stderr, "%d(%d)(%d/%d) attacking %c%d %16s damage (%d/%d) dist %d\n",
		p->p_ship->s_no,
		(int) p->p_ship->s_stats.st_kills,
		p->p_ship->s_damage,
		p->p_ship->s_shield,
		teamlet[enemy->p_ship->s_team],
		enemy->p_ship->s_no,
		enemy->p_login,
		enemy->p_ship->s_damage,
		enemy->p_ship->s_shield,
		enemy_buf->e_dist);

	if (enemy_buf->e_dist < 15000) {
	    p->p_ship->s_desdir = enemy_buf->e_course +
		    avoid[(p->p_ship->s_updates / AVOID_CLICKS) % SIZEOF(avoid)];
	    if (angdist(p->p_ship->s_desdir, p->p_ship->s_dir) > 64)
		p->p_ship->s_desspeed = 3;
	    else
		p->p_ship->s_desspeed = 4;
	} else {
	    p->p_ship->s_desdir = enemy_buf->e_course;
	    if (angdist(p->p_ship->s_desdir, p->p_ship->s_dir) > 64)
		p->p_ship->s_desspeed = 3;
	    else if (p->p_ship->s_etemp > 900)		/* 90% of 1000 */
		p->p_ship->s_desspeed = 5;
	    else
		p->p_ship->s_desspeed = 6;
	}
    } else {
	if ((enemy_buf->e_flags & (E_TEAMRED|E_TEAMYELLOW|E_PLANETPROT)) == 0)
		go_home(p);
    }
}

/*
 * Stupid Sun 3.2 tan routine calls matherr and *prints* a message if
 * it returns something other than 1.
 */
int matherr()
{
  return(1);
}

/* This function will send the robot back to it's home planet
   when it has nothing better to do.
*/
go_home(p)
struct player	*p;
{
	int x, y;
	double dx, dy;
	register struct planet	*l;
	register int		n;

	/* Find the home planet... */
	for (n = 0, l = &pdata[n]; n < MAXPLANETS; n++, l++) {
		if ((l->pl_flags & PLHOME) && l->pl_owner == p->p_ship->s_team)
			break;
	}

	if (n < MAXPLANETS) {
		l = &planets[n];	/* switch to current info */
		x = l->pl_x;
		y = l->pl_y;
		dx = x - p->p_ship->s_x;
		dy = y - p->p_ship->s_y;
		go_planet(p, l, (hypot(dx, dy) / 10000) + 1);
	}
}

/* This function will send the robot to a specified planet
   to protect it from bombing & beaming.
*/
go_planet(p, l, s)
struct player	*p;
struct planet	*l;
int		s;
{
    int x, y;
    double dx, dy;

    x = l->pl_x;
    y = l->pl_y;

    if ((ABS(x - p->p_ship->s_x) < ORBDIST) && (ABS(y - p->p_ship->s_y) < ORBDIST)) {
	p->p_ship->s_desspeed = 0;
	p->p_ship->s_flags |= SFORBIT;
	p->p_ship->s_planet = l->pl_no;
    } else {
	p->p_ship->s_flags &= ~SFORBIT;
	p->p_ship->s_desdir = newcourse(p, x, y);
	p->p_ship->s_desspeed = s;
    }
	if (p->p_ship->s_status != EXPLODE) {
		/* Self-destruct if there isn't anyone around but us robots... */
		/* or if we are not sticky */
		if (!(p->p_flags & PFRSTICKY) || (nplayers - nrobots) == 0) {
		    if (p->p_ship->s_explode == 0)
			p->p_ship->s_explode = p->p_ship->s_updates + RGIVEUPTIME;
		} else
			p->p_ship->s_explode = 0;
	}
}

projectDamage(p, eNum, dirP)
struct player	*p;
	int	*dirP;
{
	register int		i, j, numHits = 0, mx, my, tx, ty, dx, dy;
	long			tdx, tdy, mdx, mdy;
	register struct torp	*t;

	*dirP = 0;

/* XX Fix this like the tcourse computation above */
	for (i = 0, t = &torps[eNum * MAXTORP]; i < MAXTORP; i++, t++) {
		if (t->t_status == TFREE)
			continue;
		tx = t->t_x; ty = t->t_y;
		mx = p->p_ship->s_x; my = p->p_ship->s_y;
		tdx = (t->t_speed * icos[t->t_dir] * WARP1) >> TRIGSCALE;
		tdy = (t->t_speed * isin[t->t_dir] * WARP1) >> TRIGSCALE;
		mdx = (p->p_ship->s_speed * icos[p->p_ship->s_dir] * WARP1) >> TRIGSCALE;
		mdy = (p->p_ship->s_speed * isin[p->p_ship->s_dir] * WARP1) >> TRIGSCALE;
		for (j = t->t_fuse; j > 0; j--) {
			tx += tdx; ty += tdy;
			mx += mdx; my += mdy;
			dx = tx - mx; dy = ty - my;
			if (ABS(dx) < EXPDIST && ABS(dy) < EXPDIST) {
				numHits++;
				*dirP += t->t_dir;
				break;
			}
		}

	}
	if (numHits > 0)
		*dirP /= numHits;
	return (numHits);
}

unsigned char
rgetcourse(p, x, y)
struct player	*p;
int	x, y;
{
	return(iatan2(x - p->p_ship->s_x, p->p_ship->s_y - y));
}

static struct Enemy ebuf;

struct Enemy *
get_nearest(p)
register struct player *p;
{
    register struct player *j;
    register struct planet *l;
    int pcount = 0;
    register int i;
    int tdist;
    int	k;
    double dx, dy;

    /* Find an enemy */
    ebuf.e_info = p->p_ship->s_no;
    ebuf.e_dist = GWIDTH + 1;
    ebuf.e_flags = 0;

    pcount = 0;  /* number of human players in game */

    /* avoid dead slots, me */
    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if ((j->p_status != PALIVE) || (j == p))
	    continue;
	else
	    pcount++;	/* Other players in the game */

	/* Can we see them? */
	if (iscloaked(j->p_ship) && !(p->p_flags & PFRVHARD))
		continue;

	if (((j->p_ship->s_swar | j->p_ship->s_hostile) & (1 << p->p_ship->s_team))
		|| ((p->p_ship->s_swar | p->p_ship->s_hostile) & (1 << j->p_ship->s_team))) {
	    /* We have an enemy */
	    /* Get his range */
	    dx = j->p_ship->s_x - p->p_ship->s_x;
	    dy = j->p_ship->s_y - p->p_ship->s_y;
	    tdist = hypot(dx, dy);

	    if (tdist < ebuf.e_dist) {
		ebuf.e_info = i;
		ebuf.e_dist = tdist;
	    }
	}
    }
    if (pcount == 0)
	return ((struct Enemy *) NO_PLAYERS);	/* no players in game */

    if (ebuf.e_info == p->p_ship->s_no) {
	return (0);			/* no hostile players in the game */
    }


	j = &players[ebuf.e_info];

	/* Get torpedo course to nearest enemy */
	ebuf.e_flags &= ~(E_TSHOT);
	for (i = 0; i < 50; i++) {
	    double  he_x, he_y, area;

	    he_x = j->p_ship->s_x + ((icos[j->p_ship->s_dir] * j->p_ship->s_speed * WARP1) >> TRIGSCALE) * i;
	    he_y = j->p_ship->s_y + ((isin[j->p_ship->s_dir] * j->p_ship->s_speed * WARP1) >> TRIGSCALE) * i;
	    area = i * p->p_ship->s_torpspeed * WARP1;
	    if (hypot(he_x - p->p_ship->s_x, he_y - p->p_ship->s_y) < area) {
		ebuf.e_flags |= E_TSHOT;
		ebuf.e_tcourse = rgetcourse(p, (int) he_x, (int) he_y);
		break;
	    }
	}
	if (debug > 1)
	    fprintf(stderr, "torpedo course to enemy %d is %d (%d) - %s\n",
		    j->p_ship->s_no,
		    (int) ebuf.e_tcourse,
		    (int) ebuf.e_tcourse * 360 / 256,
		    (ebuf.e_flags & E_TSHOT) ? "aiming to hit" :
		    "no hit possible");

	/* Get phaser shot status */
	if (ebuf.e_dist < p->p_ship->s_phasedist)
		ebuf.e_flags |= E_PSHOT;
	else
		ebuf.e_flags &= ~(E_PSHOT);

	/* get course info */
	ebuf.e_course = rgetcourse(p, j->p_ship->s_x, j->p_ship->s_y);
	if (debug > 1)
	    fprintf(stderr, "Set course to enemy is %d (%d)\n",
		    (int) ebuf.e_course,
		    (int) ebuf.e_course * 360 / 256);

	/* Determine if there is a team member we should protect. */
	for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
		if ((j->p_status != PALIVE) || j->p_ship->s_team != p->p_ship->s_team || j == p)
			continue;

		if (j->p_ship->s_flags & SFRED) {
			ebuf.e_flags |= E_TEAMRED;
			p->p_ship->s_flags &= ~SFPLANETPROT;
			p->p_ship->s_planetprot = 0;
			/* Get his range */
			dx = j->p_ship->s_x - p->p_ship->s_x;
			dy = j->p_ship->s_y - p->p_ship->s_y;
			tdist = hypot(dx, dy);

			if (tdist < ebuf.e_dist) {
				ebuf.e_info = i;
				ebuf.e_dist = tdist;
				ebuf.e_pcourse = rgetcourse(p, j->p_ship->s_x, j->p_ship->s_y);
			}
		}
		if (ebuf.e_flags & E_TEAMRED)
			continue;
		if (j->p_ship->s_flags & SFYELLOW) {
			ebuf.e_flags |= E_TEAMYELLOW;
			p->p_ship->s_flags &= ~SFPLANETPROT;
			p->p_ship->s_planetprot = 0;
			/* Get his range */
			dx = j->p_ship->s_x - p->p_ship->s_x;
			dy = j->p_ship->s_y - p->p_ship->s_y;
			tdist = hypot(dx, dy);

			if (tdist < ebuf.e_dist) {
				ebuf.e_info = i;
				ebuf.e_dist = tdist;
				ebuf.e_pcourse = rgetcourse(p, j->p_ship->s_x, j->p_ship->s_y);
			}
		}
	}

	/* Determine if there is a planet that we should protect. */
	for (i = 0, l = &planets[0]; i < MAXPLANETS; i++, l++) {
		/* Our planet? */
		if (l->pl_owner != p->p_ship->s_team)
			continue;
		if (l->pl_flags & PLINVIS)
			continue;
		/* Ok, find planet that is being bombed. */
		/* This "simulates" a reponse to plea for help. */
		for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
			if ((j->p_status != PALIVE))
				continue;
			if (j->p_ship->s_planet == l->pl_no &&
			    ((j->p_ship->s_flags & SFBOMB) ||
			    ((j->p_ship->s_flags & SFBEAMDOWN) && j->p_ship->s_team != l->pl_owner))) {
				ebuf.e_planetprot = l->pl_no;
				ebuf.e_flags |= E_PLANETPROT;
				/* NOTE: We should find closest planet*/
				/* relative to our current position. */
				goto searchdone;
			}
		}
	}

	/* No planet to protect, but keep going if there WAS one that we */
	/* found earlier. */
	if (p->p_ship->s_flags & SFPLANETPROT) {
		ebuf.e_planetprot = p->p_ship->s_planetprot;
		ebuf.e_flags |= E_PLANETPROT;
	}

searchdone:
	if ((ebuf.e_flags & (E_TSHOT|E_PSHOT)) == 0)
		ebuf.e_dist = GWIDTH;

	return (&ebuf);
}

do_repair(p)
struct player	*p;
{
/* Repair if necessary (we are safe) */

    if (p->p_ship->s_damage > 0) {
	p->p_ship->s_desspeed = 0;
	cloak_off(p);
	shield_down(p);
	p->p_ship->s_desspeed = 0;
	if (p->p_ship->s_speed == 0)
	    repair(p);
	if (debug > 1)
	    fprintf(stderr, "%d) repairing damage at %d\n",
		p->p_ship->s_no,
		p->p_ship->s_damage);
	return(1);
    } else {
	return (0);
    }
}
