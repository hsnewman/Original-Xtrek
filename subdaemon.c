static char sccsid[] = "@(#)subdaemon.c	3.1";
#ifndef lint
static char *rcsid_daemon_c = "$Header: /uraid2/riedl/src/xceltrek/RCS/subdaemon.c,v 1.1 88/04/18 16:10:46 riedl Exp Locker: riedl $";
#endif	lint
/* Copyright (c) 1986 	Chris Guthrie */

#include <X11/Xlib.h>
#include <stdio.h>
#if !defined(cray)
#include <sys/types.h>
#endif
#include <sys/time.h>
#include <sys/file.h>

#if defined(hpux) || defined(cray)
#include <fcntl.h>
#endif

#include <sys/ioctl.h>
#include <signal.h>
#include <setjmp.h>
#include "defs.h"
#include "data.h"
extern long isin[], icos[];
#include "planets.h"

#define fuse(X) ((udcounter % (X)) == 0)
/* Run the game */

double sqrt();
double pow();
double hypot();

static int	pldebug = 0;
static int	plfd;
extern int	debug;

/*
 * initialize - allocate memory and initialize subdaemon
 */
initialize()
{
    register int i;
    int move();
    int reaper();

    udcounter = 0;
    bzero(players, sizeof (struct player) * MAXPLAYER);
    bzero(ships, sizeof (struct ship) * MAXPLAYER);
    for (i = 0; i < MAXPLAYER; i++) {
	players[i].p_status = PFREE;
	players[i].p_warncount = 0;
	ships[i].s_no = i;
    }

    plfd = open(PLFILE, O_RDWR, 0777);
    if (plfd < 0) {
	fprintf(stderr, "No planet file.  Restarting galaxy\n");
	initplanets();
    }
    else {
	if (read(plfd, planets, sizeof(pdata)) != sizeof(pdata)) {
	    fprintf(stderr, "Planet file wrong size.  Restarting galaxy\n");
	    initplanets();
	}
    }
    for (i = 0; i < MAXPLANETS; i++)
	planets[i].pl_drawtime = udcounter;
}

#define PLANET_ORBIT_FAC	3

initplanets()
{
	register struct planet	*p;
	register int		i, n;
	int			orbang, orbvel;

	bcopy(pdata, planets, sizeof (pdata));
	for (i = 0, p = &planets[0]; i < MAXPLANETS; i++, p++) {
		p->pl_no = i;
		p->pl_namelen = strlen(p->pl_name);
		p->pl_couptime = 0;
		p->pl_deadtime = 0;
		p->pl_info = (1 << p->pl_owner) | (1 << GOD);
		switch (p->pl_type) {
			case CLASSM:	strcpy(p->pl_tname, "ClassM");	break;
			case SUN:	strcpy(p->pl_tname, "Sun");	break;
			case MOON:	strcpy(p->pl_tname, "Moon");	break;
			case GHOST:	strcpy(p->pl_tname, "Ghost");	break;
			case DEAD:	strcpy(p->pl_tname, "Dead");	break;
		}
	}

	planets[33].pl_x = GWIDTH / 2;	/* Put Murisak in the middle */
	planets[33].pl_y = GWIDTH / 2;

	planets[33].pl_primary = 33;	/* It orbits itself..stationary */
	planets[33].pl_orbrad = 0;
	planets[33].pl_orbang = 0;
	planets[33].pl_orbvel = 0;

	/* And then the other suns */
	planets[32].pl_primary = 33;	/* Betelguese */
	planets[32].pl_orbrad = 36399;
	planets[32].pl_orbang = 32;
	planets[32].pl_orbvel = 0;

	planets[31].pl_primary = 33;	/* Kejela */
	planets[31].pl_orbrad = 36399;
	planets[31].pl_orbang = 224;
	planets[31].pl_orbvel = 0;

	planets[30].pl_primary = 33;	/* Sirius */
	planets[30].pl_orbrad = 36399;
	planets[30].pl_orbang = 160;
	planets[30].pl_orbvel = 0;

	planets[29].pl_primary = 33;	/* Sol */
	planets[29].pl_orbrad = 36399;
	planets[29].pl_orbang = 96;
	planets[29].pl_orbvel = 0;

	planets[35].pl_primary = 33;	/* Ghost 1 */
	planets[35].pl_orbrad = 39708;
	planets[35].pl_orbang = 0;
	planets[35].pl_orbvel = 0;

	planets[36].pl_primary = 33;	/* Ghost 2 */
	planets[36].pl_orbrad = 39708;
	planets[36].pl_orbang = 64;
	planets[36].pl_orbvel = 0;

	planets[37].pl_primary = 33;	/* Ghost 3 */
	planets[37].pl_orbrad = 39708;
	planets[37].pl_orbang = 128;
	planets[37].pl_orbvel = 0;

	planets[38].pl_primary = 33;	/* Ghost 4 */
	planets[38].pl_orbrad = 39708;
	planets[38].pl_orbang = 192;
	planets[38].pl_orbvel = 0;

	/* Murisak's planets */
	planets[12].pl_primary = 33;	/* Janus */
	planets[12].pl_orbrad = 8603;
	planets[15].pl_primary = 33;	/* Seritil */
	planets[15].pl_orbrad = 8603;
	planets[13].pl_primary = 33;	/* Elas */
	planets[13].pl_orbrad = 8603;
	planets[14].pl_primary = 33;	/* Sherman */
	planets[14].pl_orbrad = 8603;
	planets[16].pl_primary = 33;	/* Cheron */
	planets[16].pl_orbrad = 8603;

	/* Sol's planets */
	planets[0].pl_primary = 29;	/* Earth */
	planets[0].pl_orbrad = 8603;
	planets[7].pl_primary = 29;	/* Telos */
	planets[7].pl_orbrad = 8603;
	planets[10].pl_primary = 29;	/* Omega */
	planets[10].pl_orbrad = 8603;

	/* Sirius's planets */
	planets[1].pl_primary = 30;	/* Romulus */
	planets[1].pl_orbrad = 8603;
	planets[4].pl_primary = 30;	/* Remus */
	planets[4].pl_orbrad = 8603;
	planets[11].pl_primary = 30;	/* Rho */
	planets[11].pl_orbrad = 8603;

	/* Kejela's planets */
	planets[2].pl_primary = 31;	/* Klingus */
	planets[2].pl_orbrad = 8603;
	planets[5].pl_primary = 31;	/* Leudus */
	planets[5].pl_orbrad = 8603;
	planets[8].pl_primary = 31;	/* Tarsus */
	planets[8].pl_orbrad = 8603;

	/* Betelgeuse's planets */
	planets[3].pl_primary = 32;	/* Orion */
	planets[3].pl_orbrad = 8603;
	planets[6].pl_primary = 32;	/* Oberon */
	planets[6].pl_orbrad = 8603;
	planets[9].pl_primary = 32;	/* Umbriel */
	planets[9].pl_orbrad = 8603;

	/* Side systems */
	planets[20].pl_primary = 35;	/* Xidex */
	planets[20].pl_orbrad = 3805;
	planets[24].pl_primary = 35;	/* RigelB */
	planets[24].pl_orbrad = 3805;
	planets[19].pl_primary = 36;	/* Venar */
	planets[19].pl_orbrad = 3805;
	planets[23].pl_primary = 36;	/* Dyneb */
	planets[23].pl_orbrad = 3805;
	planets[18].pl_primary = 37;	/* Sarac */
	planets[18].pl_orbrad = 3805;
	planets[22].pl_primary = 37;	/* Eminiar */
	planets[22].pl_orbrad = 3805;
	planets[17].pl_primary = 38;	/* Dakel */
	planets[17].pl_orbrad = 3805;
	planets[21].pl_primary = 38;	/* Oldar */
	planets[21].pl_orbrad = 3805;

	/* Specials */
	planets[34].pl_primary = 33;	/* Syrinx */
	planets[34].pl_orbrad = 9265;
	planets[25].pl_primary = 0;	/* Luna */
	planets[25].pl_orbrad = 4136;
	planets[26].pl_primary = 34;	/* Shitface */
	planets[26].pl_orbrad = 9265;
	planets[27].pl_primary = 34;	/* Hell */
	planets[27].pl_orbrad = 6949;
	planets[28].pl_primary = 34;	/* Jinx */
	planets[28].pl_orbrad = 8603;
	planets[39].pl_primary = 33;	/* Spare 1 */
	planets[39].pl_orbrad = 16545;

	/* Set orbital angles and velocities for planets, and place them. */
	/* Murisak's planets */
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[12].pl_orbang = orbang;	/* Janus */
	planets[12].pl_orbvel = orbvel;
	planets[16].pl_orbang = (orbang + 1*(256/5)) % 256;	/* Cheron */
	planets[16].pl_orbvel = orbvel;
	planets[14].pl_orbang = (orbang + 2*(256/5)) % 256;	/* Sherman */
	planets[14].pl_orbvel = orbvel;
	planets[13].pl_orbang = (orbang + 3*(256/5)) % 256;	/* Elas */
	planets[13].pl_orbvel = orbvel;
	planets[15].pl_orbang = (orbang + 4*(256/5)) % 256;	/* Seritil */
	planets[15].pl_orbvel = orbvel;

	/* Sol's planets */
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[0].pl_orbang = orbang;	/* Earth */
	planets[0].pl_orbvel = orbvel;
	planets[7].pl_orbang = (orbang + 2*(256/3)) % 256;	/* Telos */
	planets[7].pl_orbvel = orbvel;
	planets[10].pl_orbang = (orbang + 1*(256/3)) % 256;	/* Omega */
	planets[10].pl_orbvel = orbvel;
	planets[25].pl_orbang = random() % 256;	/* Luna */
	planets[25].pl_orbvel = 12 * orbvel;

	/* Sirius's planets */
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[1].pl_orbang = orbang;	/* Romulus */
	planets[1].pl_orbvel = orbvel;
	planets[4].pl_orbang = (orbang + 2*(256/3)) % 256;	/* Remus */
	planets[4].pl_orbvel = orbvel;
	planets[11].pl_orbang = (orbang + 1*(256/3)) % 256;	/* Rho */
	planets[11].pl_orbvel = orbvel;

	/* Kejela's planets */
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[2].pl_orbang = orbang;	/* Klingus */
	planets[2].pl_orbvel = orbvel;
	planets[5].pl_orbang = (orbang + 2*(256/3)) % 256;	/* Leudus */
	planets[5].pl_orbvel = orbvel;
	planets[8].pl_orbang = (orbang + 1*(256/3)) % 256;	/* Tarsus */
	planets[8].pl_orbvel = orbvel;

	/* Betelgeuse's planets */
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[3].pl_orbang = orbang;	/* Orion */
	planets[3].pl_orbvel = orbvel;
	planets[6].pl_orbang = (orbang + 2*(256/3)) % 256;	/* Oberon */
	planets[6].pl_orbvel = orbvel;
	planets[9].pl_orbang = (orbang + 1*(256/3)) % 256;	/* Umbriel */
	planets[9].pl_orbvel = orbvel;

	/* Side systems */
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[20].pl_orbang = orbang;	/* Xidex */
	planets[20].pl_orbvel = orbvel;
	planets[24].pl_orbang = (orbang + 1*(256/2)) % 256;	/* RigelB */
	planets[24].pl_orbvel = orbvel;
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[19].pl_orbang = orbang;	/* Venar */
	planets[19].pl_orbvel = orbvel;
	planets[23].pl_orbang = (orbang + 1*(256/2)) % 256;	/* Dyneb */
	planets[23].pl_orbvel = orbvel;
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[18].pl_orbang = orbang;	/* Sarac */
	planets[18].pl_orbvel = orbvel;
	planets[22].pl_orbang = (orbang + 1*(256/2)) % 256;	/* Eminiar */
	planets[22].pl_orbvel = orbvel;
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[17].pl_orbang = orbang;	/* Dakel */
	planets[17].pl_orbvel = orbvel;
	planets[21].pl_orbang = (orbang + 1*(256/2)) % 256;	/* Oldar */
	planets[21].pl_orbvel = orbvel;

	/* Specials */
	planets[34].pl_orbvel = 0;	/* Syrinx */
	orbang = random() % 256;
	orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);
	planets[26].pl_orbang = orbang;	/* Shitface */
	planets[26].pl_orbvel = orbvel;
	planets[27].pl_orbang = (orbang + 2*(256/3)) % 256;	/* Hell */
	planets[27].pl_orbvel = orbvel;
	planets[28].pl_orbang = (orbang + 1*(256/3)) % 256;	/* Jinx */
	planets[28].pl_orbvel = orbvel;

	/* Spare planets. */
	planets[39].pl_orbang = random() % 256;	/* Spare 1 */
	planets[39].pl_orbvel = ((random() % PLANET_ORBIT_FAC)+1) * ((random() % 2) * 2 - 1);

	/* place the planets in their proper locations... */
	for (n = 3; n > 0; n--)	/* Do it a few times to get things right */
	for (i = MAXPLANETS-1, p = &planets[i]; i >= 0; i--, p--) {
		if (p->pl_primary != i) {
			p->pl_x = planets[p->pl_primary].pl_x + ((p->pl_orbrad * icos[p->pl_orbang]) >> TRIGSCALE);
			p->pl_y = planets[p->pl_primary].pl_y + ((p->pl_orbrad * isin[p->pl_orbang]) >> TRIGSCALE);
		}
	}
}

save_planets() {
  if (plfd > 0) {
    lseek(plfd, 0, 0);
    write(plfd, planets, sizeof(pdata));
  }
}

/* These specify how often special actions will take place in
   UPDATE units (0.20 seconds, currently.)
*/

#define PLAYERFUSE	1
#define TORPFUSE	1
#define PHASERFUSE	1
#define TEAMFUSE	3
#define PLFIGHTFUSE	5
#define BEAMFUSE	5
#define PLANETFUSE	61	/* was 301 - too slowww */
#define REFRESHFUSE	1
#define ROBOTFUSE	1
#define	PMOVEFUSE	(1 * UPS)	/* Planets move every 6 seconds */
#define	SAVEFUSE	(360 * UPS)	/* Save planets every 5 minutes */
#define	DEBUGFUSE	(10 * UPS)	/* Debug stuff every 10 seconds */
#define	TRACTORFUSE	(1 * UPS)	/* Tractor beams broken? */

move()
{
	register int		i;
	register struct player	*p;

	if (pldebug)
		checkplanets();
	if (fuse(PLAYERFUSE)) /* Prepare team counters for update */
		tcount[FED] = tcount[ROM] = tcount[KLI] = tcount[ORI] = 0;

	for (i = 0, p = &players[0]; i < MAXPLAYER; i++, p++) {
		if (debug && fuse(DEBUGFUSE) && p->p_status != PFREE) {
			fprintf(stderr, "pno %d, p_status %d, p_flags 0x%x, s_no %d, s_numpilots %d, p_pick 0x%x\n",
				p->p_no, p->p_status, p->p_flags,
				p->p_ship->s_no, p->p_ship->s_numpilots, p->p_pick);
		}
		if (p->p_status == PMAP)
			continue;

		if (fuse(PLAYERFUSE) && (p->p_status != PFREE)) {
			if (!p->p_copilot)
				udplayer(p);
		}
		if (fuse(TRACTORFUSE) && (p->p_status == PALIVE)) {
			if (!p->p_copilot && (p->p_ship->s_flags & SFTOWED))
				tractorcheck(p);
		}
		if (pldebug)
			checkplanets();
		if (fuse(BEAMFUSE) && (p->p_status == PALIVE)) {
			if (!p->p_copilot)
				beam(p);
		}
	}
	if (pldebug)
		checkplanets();

	/* Per tick things...one time for all players */
	if (fuse(TORPFUSE)) {
		udtorps();
	}
	if (pldebug)
		checkplanets();
	if (fuse(PHASERFUSE)) {
		udphaser();
	}
	if (pldebug)
		checkplanets();
	if (fuse(TEAMFUSE)) {
		teamtimers();
	}
	if (pldebug)
		checkplanets();
	if (fuse(PLFIGHTFUSE)) {
		plfight();	/* Planet fire */
	}
	if (pldebug)
		checkplanets();
	if (fuse(PLANETFUSE)) {
		udplanets();
	}
	if (pldebug)
		checkplanets();
	if (fuse(PMOVEFUSE)) {
		moveplanets();
	}
	if (pldebug)
		checkplanets();

	/* Remaining player things... */
	for (i = 0, p = &players[0]; i < MAXPLAYER; i++, p++) {
		if (fuse(ROBOTFUSE) &&
		    (p->p_status == PALIVE) &&
		    (p->p_flags & PFROBOT)) {
			udrobot(p);
		}
		if (pldebug)
			checkplanets();
		if (fuse(REFRESHFUSE) &&
		    (p->p_status != PFREE && p->p_status != POUTFIT &&
		     p->p_status != PSETUP && p->p_status != PMAP) &&
		    !(p->p_flags & PFROBOT)) {
			/* Can the player see their window? */
			if ((p->p_flags & PFNOTSHOWING) == 0)
				redraw(p);
		}
	}
	if (pldebug)
		checkplanets();
	if (fuse(SAVEFUSE))
		save_planets();
	if (pldebug)
		checkplanets();
}

checkplanets() {
    register int i;
    register struct planet *l;
    int x;

	x = 0;
	for (i = MAXPLANETS-1, l = &planets[i]; i >= 0; i--, l--) {
		if (i == 26 || i == 27 || i == 28 || i > 33)
			continue;
		if (l->pl_flags & PLINVIS)
			x = 1;
		if (l->pl_x < 0 || l->pl_x > GWIDTH || l->pl_y < 0 || l->pl_y > GWIDTH)
			x = 1;
		if (x == 1)
			fprintf(stderr, "Planets out of order.\n");
	}
}

moveplanets()
{
    register int i;
    register struct planet *l;
    int		orbang, oldx, oldy;

    for (i = MAXPLANETS-1, l = &planets[i]; i >= 0; i--, l--) {
	if (l->pl_primary != i && l->pl_orbvel) {
		/* pl_orbvel is angle covered in 1 minute */
		l->pl_suborbang += l->pl_orbvel;
		while (abs(l->pl_suborbang) > (60 / (PMOVEFUSE / UPS))) {
			if (l->pl_suborbang < 0) {
				l->pl_suborbang += (60 / (PMOVEFUSE / UPS));
				l->pl_orbang--;
			} else {
				l->pl_suborbang -= (60 / (PMOVEFUSE / UPS));
				l->pl_orbang++;
			}
		}
		l->pl_orbang %= 256;
		if (l->pl_orbang < 0)
			l->pl_orbang += 256;
		oldx = l->pl_x;
		l->pl_x = planets[l->pl_primary].pl_x + ((l->pl_orbrad * icos[l->pl_orbang]) >> TRIGSCALE);
		oldy = l->pl_y;
		l->pl_y = planets[l->pl_primary].pl_y + ((l->pl_orbrad * isin[l->pl_orbang]) >> TRIGSCALE);
		if (oldx != l->pl_x || oldy != l->pl_y)
			l->pl_drawtime = udcounter;
	}
    }
}

udplayer(j)
register struct player *j;
{
	auto_features(j);
      switch (j->p_status) {
      case PDEAD:
	if (--j->p_ship->s_explode <= 0 && j->p_ship->s_ntorp <= 0) {
	  if (!(j->p_flags & PFROBOT)) {
		XSync(j->display, 1);
		j->p_status = POUTFIT;
	  } else {
		/* NOTE: This is the last thing done to robots. */
		j->p_status = PFREE;
		j->p_warncount = 0;
	  }
	  j->p_redrawall = 1;
	}
	break;
      case PALIVE:
	j->p_ship->s_updates++;
	if (j->p_ship->s_status == EXPLODE) {
		j->p_ship->s_flags &= ~(SFCLOAK|SFDCLOAK);
		if (j->p_ship->s_explode == PEXPTIME)
		  blowup(j);		/* damage everyone else around */
		if (--j->p_ship->s_explode <= 0) {
			death(j);
		}
	} else {
		/* Only count pilots...to get total number of ships. */
		if (j->p_ship->s_no == j->p_no)
			tcount[j->p_ship->s_team]++;

		/* cool weapons */
		cool_weapons(j);

		/* cool engine */
		cool_engines(j);

		/* Add fuel */
		add_fuel(j);

		/* repair shields */
		repair_shields(j);

		/* repair damage */
		repair_damage(j);

		/* Charge for cloaking */
		cloak_charge(j);

		/* Set status of cloaking device depending on damage */
		check_dcloak(j);

		/* Move Player in orbit */
		player_orbit(j);

		/* Move player through space */
		space_move(j);

		/* Set player's alert status */
		adjust_alert(j);
	}
	break;
      }				/* end switch */
}

tractorcheck(j)
register struct player *j;
{
	register struct ship	*tower;

	if (!j->p_ship->s_speed)	/* No chance... */
		return;

	if ((random() % 100) < (j->p_ship->s_speed * j->p_ship->s_speed)) {
		tower = &ships[j->p_ship->s_towed];
		tower->s_flags &= ~SFTOWING;
		j->p_ship->s_flags &= ~SFTOWED;
		warning(j, "Your ship broke free of the tractor beam.");
	}
}

udtorps()
{
    register int i;
    register struct torp *j;

    for (i = 0, j = &torps[i]; i < MAXPLAYER * MAXTORP; i++, j++) {
	switch (j->t_status) {
	    case TFREE:
		continue;
	    case TMOVE:
	    case TSTRAIGHT:
		j->t_x += (j->t_speed * icos[j->t_dir] * WARP1) >> TRIGSCALE;
		if (universe.flags & WRAPAROUND) {
			/* jas (Jeff Schmidt)  Wrap around torp in the x direction */
			if (j->t_x < 0)
				j->t_x = GWIDTH;
			else if (j->t_x > GWIDTH)
				j->t_x = 0;
		} else {
			if (j->t_x < 0) {
			    j->t_x = 0;
			    explode(j);
			    break;
			}
			else if (j->t_x > GWIDTH) {
			    j->t_x = GWIDTH;
			    explode(j);
			    break;
			}
		}
		j->t_y += (j->t_speed * isin[j->t_dir] * WARP1) >> TRIGSCALE;
		if (universe.flags & WRAPAROUND) {
			/* jas (Jeff Schmidt)  Wrap around torp in the y direction */
			if (j->t_y < 0)
				j->t_y = GWIDTH;
			else if (j->t_y > GWIDTH)
				j->t_y = 0;
		} else {
			if (j->t_y < 0) {
			    j->t_y = 0;
			    explode(j);
			    break;
			}
			else if (j->t_y > GWIDTH) {
			    j->t_y = GWIDTH;
			    explode(j);
			    break;
			}
		}

		/* Make sure that player torps wobble */
		if (j->t_status == TMOVE)
		    j->t_dir += (random() % 3) - 1;

		if (near(j) || (j->t_fuse-- <= 0)) {
 		    explode(j);
		}
		break;
	    case TDET:
		j->t_x += (j->t_speed * icos[j->t_dir] * WARP1) >> TRIGSCALE;
		if (j->t_x < 0)
		    j->t_x += GWIDTH;
		else if (j->t_x > GWIDTH)
		    j->t_x -= GWIDTH;
		j->t_y += (j->t_speed * isin[j->t_dir] * WARP1) >> TRIGSCALE;
		if (j->t_y < 0)
		    j->t_y += GWIDTH;
		else if (j->t_y > GWIDTH)
		    j->t_y -= GWIDTH;
		explode(j);
		break;
	    case TEXPLODE:
		if (j->t_fuse-- <= 0) {
		    j->t_status = TFREE;
		    players[j->t_owner].p_ship->s_ntorp--;
		}
		break;
	    case TOFF:
		j->t_status = TFREE;
		players[j->t_owner].p_ship->s_ntorp--;
		break;
	}
    }
}

/* See if there is someone close enough to explode for */
near(torp)
struct torp *torp;
{
    register int i;
    int dx, dy;
    register struct player *j;

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
       if (!(j->p_status == PALIVE))
	  continue;
       if (isGod(j))
		continue;
       if (torp->t_owner == j->p_ship->s_no)
	  continue;
       if ((!(torp->t_war & (1 << j->p_ship->s_team))) &&
	   (!((1 << torp->t_team) & (j->p_ship->s_swar | j->p_ship->s_hostile))))
	  continue;
       dx = torp->t_x - j->p_ship->s_x;
       dy = torp->t_y - j->p_ship->s_y;
       if (ABS(dx) > EXPDIST || ABS(dy) > EXPDIST)
	  continue;
       if (dx * dx + dy * dy < EXPDIST * EXPDIST)
	  return 1;
    }
    return 0;
}



/* Do damage to all surrounding players */

explode(torp)
struct torp *torp;
{
    register int i;
    int dx, dy, dist;
    int damage;
    register struct player *j;

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if (!(j->p_status == PALIVE))
	    continue;
	if (isGod(j))
		continue;
	/* We can only explode so many times... */
	if (j->p_ship->s_status == EXPLODE)
	    continue;
	if (torp->t_owner == j->p_ship->s_no)
	    continue;

	/* If we are at peace with them... torps don't hurt. */
	if (!(players[torp->t_owner].p_ship->s_hostile & (1 << j->p_ship->s_team))
	     && !((1 << players[torp->t_owner].p_ship->s_team) & j->p_ship->s_hostile))
		continue;

	dx = torp->t_x - j->p_ship->s_x;
	dy = torp->t_y - j->p_ship->s_y;
	if (ABS(dx) > TDAMDIST || ABS(dy) > TDAMDIST)
	    continue;
	dist = dx * dx + dy * dy;
	if (dist > TDAMDIST * TDAMDIST)
	    continue;
	if (dist > EXPDIST * EXPDIST) {
	    damage = torp->t_damage * (TDAMDIST - sqrt((double) dist)) /
		(TDAMDIST - EXPDIST);
	}
	else {
	    damage = torp->t_damage;
	}
	if (damage > 0) {
	    /* First, check to see if torp owner has started a war */
	    if (players[torp->t_owner].p_ship->s_hostile & (1 << j->p_ship->s_team)) {
		players[torp->t_owner].p_ship->s_swar |= (1 << j->p_ship->s_team);
	    }
	    /* NOTE: Peaceful players & torps don't cause damage. See above. */
	    /* Note that if a player is at peace with the victim, then
	       the torp has caused damage either accidently, or because
	       the victim was at war with, or hostile to, the player.
	       In either case, we don't consider the damage to be
	       an act of war. */

	    if (j->p_ship->s_flags & SFSHIELD) {
		j->p_ship->s_shield -= damage;
		if (j->p_ship->s_shield < 0) {
		    j->p_ship->s_damage -= j->p_ship->s_shield;
		    j->p_ship->s_shield = 0;
		}
	    }
	    else {
		j->p_ship->s_damage += damage;
	    }
	    if (j->p_ship->s_damage >= j->p_ship->s_maxdamage) {
		j->p_ship->s_status = EXPLODE;
		j->p_ship->s_explode = PEXPTIME;
		/* no points for killing yourself! */
		if (torp->t_owner != j->p_ship->s_no) {
		  /* No kills for practice robots either. */
		  if ((j->p_flags & PFPRACTICER) == 0) {
			players[torp->t_owner].p_ship->s_stats.st_kills +=  1.0
			  + j->p_ship->s_armies * 0.1 + j->p_ship->s_stats.st_kills * 0.1;
			players[torp->t_owner].p_ship->s_stats.st_armsship +=
			  j->p_ship->s_armies;
		        update_weapons(&players[torp->t_owner]);
		  }
		}
		killmess(j, &players[torp->t_owner]);
		j->p_ship->s_whydead = KTORP;
		j->p_ship->s_whodead = torp->t_owner;
	    }
	}
    }
    torp->t_status = TEXPLODE;
    torp->t_fuse = TEXPTIME;
}

udplanets()
{
    register int i;
    register struct planet *l;

    for (i = 0, l = &planets[i]; i < MAXPLANETS; i++, l++) {
	if (l->pl_armies == 0)
	    continue;
	if (l->pl_type == SUN || l->pl_type == MOON || l->pl_type == GHOST)
		continue;
#ifdef notdef  /* Looks like old attempt at plague. */
	if ((random() % 3000) < l->pl_armies)
	    l->pl_armies -= (random() % l->pl_armies);
#endif
	if ((l->pl_armies < 4) && ((random() % (l->pl_type == CLASSM ? 10 : 20)) == 0)) {
	    l->pl_armies++;
	    continue;
	}
	if ((random() % (l->pl_type == CLASSM ? 10 : 20)) == 0)
	    l->pl_armies += (random() % 3) + 1;

	if (l->pl_armies > (l->pl_type == CLASSM ? 125 : 75)) {
		if ((random() % 90) == 0) {
			/* Plague.. */
			l->pl_armies = (random() % 20) + 5;
		}
	}
    }
}

udphaser()
{
    register int i;
    register struct phaser *j;
    register struct player *victim;

    for (i = 0, j = &phasers[i]; i < MAXPLAYER; i++, j++) {
	switch (j->ph_status) {
	    case PHFREE:
		continue;
	    case PHMISS:
		if (j->ph_fuse-- == 1)
		    j->ph_status = PHFREE;
		break;
	    case PHHIT:
		if (j->ph_fuse-- == PFIRETIME) {
		    victim = &players[j->ph_target];
		    if (isGod(victim)) continue;
		    /* the victim may have been killed by a torp already */
		    if (victim->p_status != PALIVE) continue;
		    if (victim->p_ship->s_status == EXPLODE) continue;

		    if (victim->p_ship->s_flags & SFSHIELD) {
			victim->p_ship->s_shield -= j->ph_damage;
			if (victim->p_ship->s_shield < 0) {
			    victim->p_ship->s_damage -= victim->p_ship->s_shield;
			    victim->p_ship->s_shield = 0;
			}
		    }
		    else {
			victim->p_ship->s_damage += j->ph_damage;
		    }
		    if (victim->p_ship->s_damage >= victim->p_ship->s_maxdamage) {
			victim->p_ship->s_status = EXPLODE;
			victim->p_ship->s_explode = PEXPTIME;
			if ((victim->p_flags & PFPRACTICER) == 0) {
			  players[i].p_ship->s_stats.st_kills += 1.0
			    + victim->p_ship->s_armies * 0.1 + victim->p_ship->s_stats.st_kills * 0.1;
			  players[i].p_ship->s_stats.st_armsship += victim->p_ship->s_armies;
			  update_weapons(&players[i]);
			}
			killmess(victim, &players[i]);
			victim->p_ship->s_whydead = KPHASER;
			victim->p_ship->s_whodead = i;
		    }
		}
		if (j->ph_fuse == 0)
		    j->ph_status = PHFREE;
		break;
	}
    }
}

int pl_warning[MAXPLANETS];	/* To keep planets shut up for awhile */
int tm_robots[MAXTEAM + 1];	/* To limit the number of robots */
int tm_coup[MAXTEAM + 1];	/* To allow a coup */

teamtimers()
{
    register int i;
    for (i = 0; i <= MAXTEAM; i++) {
	if (tm_robots[i] > 0)
	    tm_robots[i]--;
	if (tm_coup[i] > 0)
	    tm_coup[i]--;
    }
}

plfight()
{
    register int h, i;
    register struct player *j;
    register struct planet *l;
    int dx, dy;
    int damage;
    int dist;
    int rnd;
    char buf[80];
    char buf1[80];

    for (h = 0, l = &planets[h]; h < MAXPLANETS; h++, l++) {
	if (l->pl_flags & PLCOUP) {
	    l->pl_flags &= ~PLCOUP;
	    l->pl_owner = pdata[h].pl_owner;
	    /* Make sure the newly coup'ed planet has some armies. */
	    if (l->pl_armies <= 0) {
		l->pl_armies = (random() % 5) + 1;
	    }
	}
	if (pl_warning[h] > 0)
	    pl_warning[h]--;
	for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	    switch (j->p_status) {
		case PFREE:
		case PDEAD:
		    continue;
		case PALIVE:
		    if (j->p_copilot)
			continue;
		    /* Do damage by planets */
			if (l->pl_flags & PLINVIS) {
				/* Only visible planets do damage... */
				continue;
			}
			dx = ABS(l->pl_x - j->p_ship->s_x);
			dy = ABS(l->pl_y - j->p_ship->s_y);
			if (dx < 3 * PFIREDIST && dy < 3 * PFIREDIST && !isGod(j))
			    l->pl_drawtime = udcounter;
			if (j->p_ship->s_status == EXPLODE)
				continue;
			if (dx > PFIREDIST || dy > PFIREDIST)	/*XXX*/
			    continue;
			dist = (int) hypot((double) dx, (double) dy);
			if (dist > PFIREDIST)
			    continue;
			/* If they are close enough, they get information. */
			if (dist <= ORBDIST)
			    l->pl_info |= (1 << j->p_ship->s_team);
		    if (((j->p_ship->s_swar | j->p_ship->s_hostile) & (1 << l->pl_owner)) ||
			l->pl_owner == SELFRULED || l->pl_type == SUN) {
			if (l->pl_armies > 0) {
			    damage = l->pl_armies / 30 + (random() % 4);
			    if (damage > 8)
				damage = 8;
			} else
			    damage = 0;
			if (isGod(j))
				damage = 0;
			if (damage > 0) {
			    if (j->p_ship->s_flags & SFSHIELD) {
				j->p_ship->s_shield -= damage;
				if (j->p_ship->s_shield < 0) {
				    j->p_ship->s_damage -= j->p_ship->s_shield;
				    j->p_ship->s_shield = 0;
				}
			    }
			    else {
				j->p_ship->s_damage += damage;
			    }
			    if (j->p_ship->s_damage >= j->p_ship->s_maxdamage) {
			    j->p_ship->s_explode = PEXPTIME;
			    j->p_ship->s_status = EXPLODE;
			    sprintf(buf, "%s (%c%x) killed by %s (%c)",
				j->p_name,
				teamlet[j->p_ship->s_team],
				j->p_ship->s_no,
				l->pl_name,
				teamlet[l->pl_owner]);
			    pmessage(buf, 0, MALL, "GOD->ALL");
			    j->p_ship->s_whydead = KPLANET;
			    j->p_ship->s_whodead = h;
			    }
			}
		    }
		    /* do bombing */
		    if ((!(j->p_ship->s_flags & SFORBIT)) || (!(j->p_ship->s_flags & SFBOMB)))
			continue;
		    if (j->p_ship->s_planet != l->pl_no)
			continue;
		    if (!((j->p_ship->s_swar | j->p_ship->s_hostile) & (1 << l->pl_owner)) &&
			l->pl_owner != SELFRULED)
			continue;
		    if (l->pl_armies < 5)
			continue;

			/* Warn owning team */
		    if (pl_warning[h] <= 0) {
			pl_warning[h] = 50/PLFIGHTFUSE;
			sprintf(buf, "We are under attack.  Please send aid");
			sprintf(buf1, "%-3s->%-3s",
			    l->pl_name, teamshort[l->pl_owner]);
			pmessage(buf, l->pl_owner, MTEAM, buf1);
		    }

		    damage = j->p_ship->s_torpdamage;
		    damage *= damage;
		    damage /= 200;

		    rnd = random() % 100;
		    if (rnd < 50 || damage <= 0) {
			continue;
		    }
		    damage = random() % damage;
		    if (damage <= 0)
			continue;
		    if (damage > l->pl_armies)
			damage = l->pl_armies;
		    l->pl_armies -= damage;
		    j->p_ship->s_stats.st_kills += (0.02 * damage);
		    j->p_ship->s_stats.st_armsbomb += damage;

		    /* Send in a robot if there are no other defenders
			and the planet is in the team's home space */
		    /* Send in an easy robot if the planet is not in */
		    /*  the team's home space. */
		    /* Send in robots until there are the same number */
		    /* of defenders as attackers. */
		    /* But don't start them up all at once
		       (tm_robots is a timer). */

		    /* NOTE: It would be nice to send in an extra robot if */
		    /* there are only 1 or 2 team planets left.  Later. */

		    if ((tcount[l->pl_owner] < tcount[j->p_ship->s_team]) &&
			(pdata[h].pl_owner == l->pl_owner &&
			 l->pl_owner != SELFRULED) &&
			    tm_robots[l->pl_owner] == 0) {
				startrobot(l->pl_owner, PFRSTICKY|PFRHARD);
				tm_robots[l->pl_owner] = (60 +
				    (random() % 60)) /
				    TEAMFUSE;
		    } else if ((tcount[l->pl_owner] < tcount[j->p_ship->s_team]) &&
			    l->pl_owner != SELFRULED &&
			    tm_robots[l->pl_owner] == 0) {
				startrobot(l->pl_owner, PFRSTICKY|PFPRACTICER);
				tm_robots[l->pl_owner] = (60 +
				    (random() % 60)) /
				    TEAMFUSE;
		    }

	    }
	}
    }
}

/* udrobot - Check if any of the robots in the player list deserve
 * their chance to execute this tick.  Note that hard robots
 * get to recalculate more often.  In order not to kill the game
 * they only recalculate every other call; the other times they just
 * fire an additional torp.
 */
udrobot(j)
register struct player *j;
{
	if (j->p_flags & PFRVHARD) { /* very hard robots recalculate every turn */
	    rmove(j);
	} else if (j->p_flags & PFRHARD) { /* hard robots recalculate more often */
	    if (udcounter % 4 == j->p_ship->s_no % 4) rmove(j);
	} else {
	    if (udcounter % 10 == j->p_ship->s_no % 10) rmove(j);
	}
}

beam(j)
register struct player *j;
{
    register int h, i;
    register struct planet *l;
    char buf[132];
    int		old_owner;

	/* do beaming */
	if (!(j->p_ship->s_flags & SFORBIT)) {
		j->p_ship->s_flags &= ~(SFBEAMUP|SFBEAMDOWN);
		return;
	}

      for (h = 0, l = &planets[h]; h < MAXPLANETS; h++, l++) {
	    if (j->p_ship->s_planet != l->pl_no)
		continue;
	    if (j->p_ship->s_flags & SFBEAMUP) {
		if (l->pl_armies < 5)
		    continue;
		if (j->p_ship->s_armies == j->p_ship->s_maxarmies)
		    continue;
		/* XXX */
		if (!isGod(j) && j->p_ship->s_armies == floor(j->p_ship->s_stats.st_kills * 2.0))
		    continue;
		if (!isGod(j) && j->p_ship->s_team != l->pl_owner)
		    continue;
		j->p_ship->s_armies++;
		l->pl_armies--;
	    }
	    if (j->p_ship->s_flags & SFBEAMDOWN) {
		if (j->p_ship->s_armies == 0)
		    continue;
		if (!isGod(j) && j->p_ship->s_team != l->pl_owner) {
		    j->p_ship->s_armies--;
		    if (l->pl_armies) {
			l->pl_armies--;
			j->p_ship->s_stats.st_kills += 0.02;
			j->p_ship->s_stats.st_armsbomb++;
		    }
		    else { 	/* planet taken over */
			l->pl_armies++;
			old_owner = l->pl_owner;
			l->pl_owner = j->p_ship->s_team;
			j->p_ship->s_stats.st_planets++;
			j->p_ship->s_stats.st_kills += 0.25;
			l->pl_info = (1 << j->p_ship->s_team);
			l->pl_info |= (1 << GOD);
			sprintf(buf, "%s taken over by %s (%c%x)",
			    l->pl_name,
			    j->p_name,
			    teamlet[j->p_ship->s_team],
			    j->p_ship->s_no);
			pmessage(buf, 0, MALL, "GOD->ALL");
			/* Don't wait for the fuse */
			save_planets();
			/* checkwin also checks & updates for genocides. */
			checkwin(j, old_owner);
		    }
		} else {
		    j->p_ship->s_armies--;
		    l->pl_armies++;
		}
	    }
	}
}

blowup(sh)
struct player *sh;
{
    register int i;
    int dx, dy, dist;
    int damage;
    register struct player *j;

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if (j->p_status == PFREE || j->p_status == PDEAD || (j->p_status == PALIVE && j->p_ship->s_status == EXPLODE))
	    continue;
	if (sh == j)
	    continue;
	if (isGod(j))
		continue;
	dx = sh->p_ship->s_x - j->p_ship->s_x;
	dy = sh->p_ship->s_y - j->p_ship->s_y;
	if (ABS(dx) > TDAMDIST || ABS(dy) > TDAMDIST)
	    continue;
	dist = dx * dx + dy * dy;
	if (dist > TDAMDIST * TDAMDIST)
	    continue;
	if (dist > EXPDIST * EXPDIST) {
	    damage = j->p_ship->s_maxdamage * (TDAMDIST - sqrt((double) dist)) /
		(TDAMDIST - EXPDIST);
	}
	else {
	    damage = j->p_ship->s_maxdamage;
	}
	if (damage > 0) {
	    if (j->p_ship->s_flags & SFSHIELD) {
		j->p_ship->s_shield -= damage;
		if (j->p_ship->s_shield < 0) {
		    j->p_ship->s_damage -= j->p_ship->s_shield;
		    j->p_ship->s_shield = 0;
		}
	    }
	    else {
		j->p_ship->s_damage += damage;
	    }
	    if (j->p_ship->s_damage >= j->p_ship->s_maxdamage) {
		j->p_ship->s_status = EXPLODE;
		j->p_ship->s_explode = PEXPTIME;
		if ((j->p_flags & PFPRACTICER) == 0) {
		  sh->p_ship->s_stats.st_kills +=
			1.0 + j->p_ship->s_armies * 0.1 + j->p_ship->s_stats.st_kills * 0.1;
		  sh->p_ship->s_stats.st_armsship += j->p_ship->s_armies;
		  /* Don't update_weapons(sh); as sh is blowing up... */
		}
		killmess(j, sh);
		j->p_ship->s_whydead = KSHIP;
		j->p_ship->s_whodead = sh->p_ship->s_no;
	    }
	}
    }
}

/* This function is called when a planet has been taken over.
   It checks all the planets to see if the victory conditions
   are right.  If so, it blows everyone out of the game and
   resets the galaxy
   As a subfunction, it checks for genocides.
*/
checkwin(winner, old_owner)
struct player *winner;
int	old_owner;
{
    register int i, h;
    register struct planet *l;
    register struct player *j;
    int team[MAXTEAM + 1];
    char	buf[256];

    for (i = 0; i < NUMTEAM; i++)
	team[i] = 0;

    for (i = 0, l = &planets[i]; i < MAXPLANETS; i++, l++)
	team[l->pl_owner]++;

    /* First, check for a genocide. */
    if (team[old_owner] == 0) {
	/* Yup...just wiped them out. */
	winner->p_ship->s_stats.st_genocides++;
	sprintf(buf, "The %s was genocided by %s (%c%x)",
		teamlong[old_owner], winner->p_name,
		teamlet[winner->p_ship->s_team], winner->p_ship->s_no);
	pmessage(buf, 0, MALL, "GOD->ALL");
    }

    for (i = 0; i < NUMTEAM; i++) {
	if (team[i] == NUMCONPLANETS) {
	    /* We have a winning team */
	    for (h = 0, j = &players[0]; h < MAXPLAYER; h++, j++) {
		if (j->p_status == PFREE)
			continue;
		j->p_ship->s_whydead = KWINNER;
		j->p_ship->s_whodead = winner->p_ship->s_no;
		death(j);
		j->p_pick = ((1<<FED)|(1<<ROM)|(1<<KLI)|(1<<ORI));	/* Allow them to repick any team */
		j->p_ship->s_team = 0;
	    }
	    winner->p_ship->s_stats.st_conqs++;
	    initplanets();
	    save_planets();
	}
    }
}

killmess(victim, killer)
struct player *victim, *killer;
{
    register char *cp;
    char buf[80];

    buf[0] = '\0';
    if (victim->p_ship->s_numpilots == 1)
	sprintf(buf, "%s ", victim->p_name);
    cp = &buf[strlen(buf)];

    *cp++ = '(';
    *cp++ = teamlet[victim->p_ship->s_team];
    if (victim->p_ship->s_no <= 9)
	    *cp++ = victim->p_ship->s_no + '0';
    else
	    *cp++ = victim->p_ship->s_no + 'A' - 10;
    *cp++ = ')';

    sprintf(cp, " was kill %0.2f for", killer->p_ship->s_stats.st_kills);

    cp = &buf[strlen(buf)];
    if (killer->p_ship->s_numpilots == 1) {
	*cp++ = ' ';
	strcpy(cp, killer->p_name);
    }

    cp = &buf[strlen(buf)];
    *cp++ = ' ';
    *cp++ = '(';
    *cp++ = teamlet[killer->p_ship->s_team];
    if (killer->p_ship->s_no <= 9)
	    *cp++ = killer->p_ship->s_no + '0';
    else
	    *cp++ = killer->p_ship->s_no + 'A' - 10;
    *cp++ = ')';
    *cp = '\0';
    pmessage(buf, 0, MALL, "GOD->ALL");
}

dumpmessages()
{
    register int i;
    register struct message *j;

    for (i = 0, j = &messages[0]; i < MAXMESSAGE; i++, j++)
	if (j->m_flags & MVALID)
	    printf("%d, %s\n", i, j->m_data);
}

startrobot(team, flags)
int team;
int flags;
{
    register struct player	*p;
    int rpno;
    extern char *rnames[4];
    extern int playerchange;

    if ((rpno = findslot()) >= MAXPLAYER) return;
    p = &players[rpno];
    p->p_ship = &ships[rpno];
    p->p_ship->s_numpilots = 1;
    p->p_ship->s_team = team;
    playerchange = 1;
    strncpy(p->p_login, "Robot", strlen("Robot"));
    p->p_login[strlen("Robot")] = NULL;
    strncpy(p->p_name, rnames[team], strlen(rnames[team]));
    p->p_name[strlen(rnames[team])] = NULL;
    if (flags & PFRHARD)
	strcat(p->p_name, "-II");
    else if (flags & PFRVHARD)
	strcat(p->p_name, "-III");
    else
	strcat(p->p_name, "-I");
    enter(team, "Nowhere", rpno);
    p->p_status = PALIVE;
    p->p_flags |= PFROBOT;		/* Mark as a robot */
    if (p->p_ship->s_flags & SFTOWED) {
	ships[p->p_ship->s_towed].s_flags &= ~SFTOWING;
    }
    tow_off(p);
    p->p_ship->s_phasercost = 0;
    p->p_ship->s_torpcost = 0;
    /* Set robot difficulty */
    p->p_flags &= ~(PFRHOSTILE|PFRHARD|PFRVHARD);
    p->p_flags |= flags;
    if (p->p_flags & PFRHOSTILE)
	p->p_ship->s_hostile |= (1 << team);
}

