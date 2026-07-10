static char sccsid[] = "@(#)minisubs.c	3.1";
#include <X11/Xlib.h>
#include "defs.h"
#include "data.h"

extern long isin[], icos[];

cool_weapons(who)
register struct player	*who;
{
	if (isGod(who)) {
		who->p_ship->s_wtemp = 0;
		who->p_ship->s_flags &= ~SFWEP;
		return;
	}
	who->p_ship->s_wtemp -= who->p_ship->s_wcool;
	if (who->p_ship->s_wtemp < 0)
		who->p_ship->s_wtemp = 0;
	if (who->p_ship->s_flags & SFWEP) {
		if (--who->p_ship->s_wtime <= 0) {
			who->p_ship->s_flags &= ~SFWEP;
			warning(who, "Weapons are back on-line.");
		}
	} else if (who->p_ship->s_wtemp > 1000) {
		if (!(random() % 40)) {
			who->p_ship->s_flags |= SFWEP;
			who->p_ship->s_wtime = (random() % PWEAPLOCKVAR) + PWEAPLOCKMIN;
			warning(who, "Weapons overload.");
		}
	}
}

cool_engines(who)
register struct player	*who;
{
	if (isGod(who)) {
		who->p_ship->s_etemp = 0;
		who->p_ship->s_flags &= ~SFENG;
		return;
	}
	who->p_ship->s_etemp -= who->p_ship->s_ecool;
	if (who->p_ship->s_etemp < 0)
		who->p_ship->s_etemp = 0;
	if (who->p_ship->s_flags & SFENG) {
		if (--who->p_ship->s_etime <= 0) {
			who->p_ship->s_flags &= ~SFENG;
			warning(who, "Engines are now functional.");
		}
	} else if (who->p_ship->s_etemp > 1000) {
		if (!(random() % 40)) {
			who->p_ship->s_flags |= SFENG;
			who->p_ship->s_etime = (random() % PENGLOCKVAR) + PENGLOCKMIN;
			who->p_ship->s_desspeed = 0;
			warning(who, "Engines super-heated.");
		}
	}
}

add_fuel(who)
register struct player *who;
{
	int	amt;

	if (isGod(who)) {
		who->p_ship->s_fuel = who->p_ship->s_maxfuel;
		return;
	}
	amt = who->p_ship->s_recharge;

	/* You get 25% more for having shields down. */
	if (!(who->p_ship->s_flags & SFSHIELD))
		amt += (who->p_ship->s_recharge / 4);

	/* You get more fuel for orbiting friendly, populated class M planets */
	/* with shields down. */
	if ((who->p_ship->s_flags & SFORBIT) &&
	    (planets[who->p_ship->s_planet].pl_type == CLASSM) &&
	    !((1 << planets[who->p_ship->s_planet].pl_owner) & (who->p_ship->s_swar | who->p_ship->s_hostile)) &&
	    (planets[who->p_ship->s_planet].pl_armies > 0) &&
	    !(who->p_ship->s_flags & SFSHIELD)) {
		amt *= 8;
	} else
		amt *= 2;

	who->p_ship->s_fuel += amt;
	if (who->p_ship->s_fuel > who->p_ship->s_maxfuel)
		who->p_ship->s_fuel = who->p_ship->s_maxfuel;
}

repair_shields(who)
register struct player *who;
{
	if (isGod(who)) {
		who->p_ship->s_shield = who->p_ship->s_maxshields;
		return;
	}
	/* NOTE: Shields repair twice as fast when they are down. */

        if (who->p_ship->s_shield < who->p_ship->s_maxshields) {
          if ((who->p_ship->s_flags & SFREPAIR) && (who->p_ship->s_speed == 0)) {
	    if (who->p_ship->s_flags & SFSHIELD)
		    who->p_ship->s_subshield += who->p_ship->s_repair * 4;
	    else
		    who->p_ship->s_subshield += who->p_ship->s_repair * 8;
            if ((who->p_ship->s_flags & SFORBIT) &&
                (planets[who->p_ship->s_planet].pl_armies > 0) &&
                (!((1 << planets[who->p_ship->s_planet].pl_owner)
                   & (who->p_ship->s_swar | who->p_ship->s_hostile)))) {
		if (who->p_ship->s_flags & SFSHIELD)
			who->p_ship->s_subshield += who->p_ship->s_repair * 4;
		else
			who->p_ship->s_subshield += who->p_ship->s_repair * 8;
              }
          } else {
	     if (who->p_ship->s_flags & SFSHIELD)
		    who->p_ship->s_subshield += who->p_ship->s_repair * 2;
	     else
		    who->p_ship->s_subshield += who->p_ship->s_repair * 4;
	  }

          if (who->p_ship->s_subshield / 1000) {
            who->p_ship->s_shield += who->p_ship->s_subshield / 1000;
            who->p_ship->s_subshield %= 1000;
          }
          if (who->p_ship->s_shield > who->p_ship->s_maxshields) {
            who->p_ship->s_shield = who->p_ship->s_maxshields;
            who->p_ship->s_subshield = 0;
          }
        }
}

repair_damage(who)
register struct player *who;
{
	if (isGod(who)) {
		who->p_ship->s_flags &= ~SFREPAIR;
		who->p_ship->s_damage = 0;
		return;
	}
        if (who->p_ship->s_damage && !(who->p_ship->s_flags & SFSHIELD)) {
          if ((who->p_ship->s_flags & SFREPAIR) && (who->p_ship->s_speed == 0)) {
            who->p_ship->s_subdamage += who->p_ship->s_repair * 2;
            if ((who->p_ship->s_flags & SFORBIT) &&			/* orbiting */
                (planets[who->p_ship->s_planet].pl_armies > 0) &&	/* a populated*/
                (!((1 << planets[who->p_ship->s_planet].pl_owner)	/* friendly */
                   & (who->p_ship->s_swar | who->p_ship->s_hostile)))) {	/* planet */
                who->p_ship->s_subdamage += who->p_ship->s_repair * 2;
              }
          } else
            who->p_ship->s_subdamage += who->p_ship->s_repair;
          if (who->p_ship->s_subdamage / 1000) {
            who->p_ship->s_damage -= who->p_ship->s_subdamage / 1000;
            who->p_ship->s_subdamage %= 1000;
          }
          if (who->p_ship->s_damage < 0) {
	    who->p_ship->s_flags &= ~SFREPAIR;
            who->p_ship->s_damage = 0;
            who->p_ship->s_subdamage = 0;
          }
        }
}

check_dcloak(who)
register struct player *who;
{
	if (isGod(who)) {
		/* Never... */
		return;
	}
        if ((!(who->p_ship->s_flags & SFCLOAK)) || who->p_ship->s_damage < who->p_ship->s_dcloak) {
		who->p_ship->s_flags &= ~SFDCLOAK;
		return;
	}
	/* Ok, we are over the threshold... */
	if (who->p_ship->s_flags & SFDCLOAK) {
		/* The cloaks are damaged, see if they should become "undamaged" */
		if ((random() % 100) < who->p_ship->s_dcloak_off) {
			who->p_ship->s_flags &= ~SFDCLOAK;
		}
	} else {
		/* The cloaks are good, see if they should become "damaged" */
		if ((random() % 100) < who->p_ship->s_dcloak_on) {
			who->p_ship->s_flags |= SFDCLOAK;
		}
	}
}

cloak_charge(who)
register struct player *who;
{
	if (isGod(who)) {
		return;
	}
        if (who->p_ship->s_flags & SFCLOAK) {
	  who->p_ship->s_fuel -= (who->p_ship->s_cloakcost * (who->p_ship->s_speed+1));
        }

	if (who->p_ship->s_fuel <= 0) {
		who->p_ship->s_fuel = 0;
		tow_off(who);
		who->p_ship->s_desspeed = 0;
		cloak_off(who);
	}
}

player_orbit(who)
register struct player *who;
{
        if (who->p_ship->s_flags & SFORBIT) {
          who->p_ship->s_dir += 2;
          who->p_ship->s_desdir = who->p_ship->s_dir;
          who->p_ship->s_x = planets[who->p_ship->s_planet].pl_x +
            ((ORBDIST * icos[(unsigned char) (who->p_ship->s_dir - (unsigned char) 64)])
                                                >> TRIGSCALE);
          who->p_ship->s_y = planets[who->p_ship->s_planet].pl_y +
            ((ORBDIST * isin[(unsigned char) (who->p_ship->s_dir - (unsigned char) 64)])
                                                >> TRIGSCALE);
        }
}

space_move(who)
register struct player *who;
{
	register struct ship	*ship, *towee;
	int		delta_x, delta_y;
	unsigned char	dir_of_pull;
	extern unsigned char	iatan2();
	extern double	hypot();

	ship = who->p_ship;

	/* Don't move ships that are orbitting */
	if (!(ship->s_flags & SFORBIT)) {
		if (ship->s_dir != ship->s_desdir)
			changedir(who);

		/* Alter speed */
		alter_speed(who);

		/* Charge for speed */
		charge_speed(who);

		/* For ships that are being towed, don't move them. */
		/* They get moved by ships that are towing them. */
		if (ship->s_flags & SFTOWED)
			return;

		/* Move them... */
		delta_x = (ship->s_speed * icos[ship->s_dir] * WARP1) >> TRIGSCALE;
		ship->s_x += delta_x;
		delta_y = (ship->s_speed * isin[ship->s_dir] * WARP1) >> TRIGSCALE;
		ship->s_y += delta_y;

		check_sides(ship);

		/* If towing someone, move the ship */
		if (ship->s_flags & SFTOWING) {
			towee = &ships[ship->s_towing];
			dir_of_pull = iatan2((ship->s_x - towee->s_x) - who->p_xwinsize / 2, who->p_ywinsize / 2 - (ship->s_y - towee->s_y));
			towee->s_desdir = dir_of_pull;

			delta_x = (ship->s_speed * icos[dir_of_pull] * WARP1) >> TRIGSCALE;
			delta_y = (ship->s_speed * isin[dir_of_pull] * WARP1) >> TRIGSCALE;
			towee->s_x += delta_x;
			towee->s_y += delta_y;
			check_sides(towee);
		}
	}
}

check_sides(ship)
register struct ship	*ship;
{
	if (universe.flags & WRAPAROUND) {
		/*
		 * jas (Jeff Schmidt)  Let the ships wrap around the universe.
		 */

		if (ship->s_x < 0)
			ship->s_x = GWIDTH;
		else if (ship->s_x > GWIDTH)
			ship->s_x = 0;

		if (ship->s_y < 0)
			ship->s_y = GWIDTH;
		else if (ship->s_y > GWIDTH)
			ship->s_y = 0;
	} else {
		/* Bounce off the side of the galaxy */
		if (ship->s_x < 0) {
			ship->s_x = -ship->s_x;
			if (ship->s_dir == 192)
				ship->s_dir = ship->s_desdir = 64;
			else
				ship->s_dir = ship->s_desdir = 64 - (ship->s_dir - 192);
		} else if (ship->s_x > GWIDTH) {
			ship->s_x = GWIDTH - (ship->s_x - GWIDTH);
			if (ship->s_dir == 64)
				ship->s_dir = ship->s_desdir = 192;
			else
				ship->s_dir = ship->s_desdir = 192 - (ship->s_dir - 64);
		}

		if (ship->s_y < 0) {
			ship->s_y = -ship->s_y;
			if (ship->s_dir == 0)
				ship->s_dir = ship->s_desdir = 128;
			else
				ship->s_dir = ship->s_desdir = 128 - ship->s_dir;
		} else if (ship->s_y > GWIDTH) {
			ship->s_y = GWIDTH - (ship->s_y - GWIDTH);
			if (ship->s_dir == 128)
				ship->s_dir = ship->s_desdir = 0;
			else
				ship->s_dir = ship->s_desdir = 0 - (ship->s_dir - 128);
		}
	}
}

#define YRANGE ((GWIDTH)/5)
#define RRANGE ((GWIDTH)/10)

adjust_alert(who)
register struct player *who;
{
	register int	k;
	register int	dx, dy, dist;
	register struct player	*j;

        who->p_ship->s_flags |= SFGREEN;
        who->p_ship->s_flags &= ~(SFRED|SFYELLOW);
        for (k = 0, j = &players[0]; k < MAXPLAYER; k++, j++) {
	  if (j == who)	/* Don't count yourself in the alert status */
		continue;
          if ((j->p_status == PALIVE) &&
              (((who->p_ship->s_swar | who->p_ship->s_hostile) & (1 << j->p_ship->s_team)) ||
	       ((j->p_ship->s_swar | j->p_ship->s_hostile) & (1 << who->p_ship->s_team)))) {
            dx = who->p_ship->s_x - j->p_ship->s_x;
            dy = who->p_ship->s_y - j->p_ship->s_y;
            if (ABS(dx) > YRANGE || ABS(dy) > YRANGE)
              continue;
            dist = dx * dx + dy * dy;
            if (dist <  RRANGE * RRANGE) {
              who->p_ship->s_flags |= SFRED;
              who->p_ship->s_flags &= ~(SFGREEN|SFYELLOW);
            } else if ((dist <  YRANGE * YRANGE) &&
                     (!(who->p_ship->s_flags & SFRED))) {
                     who->p_ship->s_flags |= SFYELLOW;
                     who->p_ship->s_flags &= ~(SFGREEN|SFRED);
	    }
          }
        }
}

alter_speed(who)
register struct player *who;
{
	register int	maxspeed;

	if (isGod(who)) {
		who->p_ship->s_speed = who->p_ship->s_desspeed;
		return;
	}
	maxspeed = who->p_ship->s_maxspeed;
        maxspeed = maxspeed - ((who->p_ship->s_damage * maxspeed) /
				(who->p_ship->s_maxdamage));

	if (maxspeed > who->p_ship->s_maxspeed)
		maxspeed = who->p_ship->s_maxspeed;

	/* At this point...it no longer is "maxspeed", but "desspeed" */
	if (maxspeed > who->p_ship->s_desspeed)
		maxspeed = who->p_ship->s_desspeed;

          if (maxspeed > who->p_ship->s_speed) {
            who->p_ship->s_subspeed += who->p_ship->s_accint * (10 / UPS);
          }
          if (maxspeed < who->p_ship->s_speed) {
            who->p_ship->s_subspeed -= who->p_ship->s_decint * (10 / UPS);
          }
          if (who->p_ship->s_subspeed / 1000) {
            who->p_ship->s_speed += who->p_ship->s_subspeed / 1000;
            who->p_ship->s_subspeed /= 1000;
            if (who->p_ship->s_speed < 0)
              who->p_ship->s_speed = 0;
            if (who->p_ship->s_speed > who->p_ship->s_maxspeed)
              who->p_ship->s_speed = who->p_ship->s_maxspeed;
          }
}

charge_speed(who)
register struct player *who;
{
	register struct player *towee;
	int	amt;

	if (isGod(who)) {
		return;
	}
	amt = who->p_ship->s_warpcost * who->p_ship->s_speed;
	who->p_ship->s_etemp += who->p_ship->s_speed;

	/* Shields up cost 50% more */
	if (who->p_ship->s_flags & SFSHIELD) {
		amt += ((who->p_ship->s_warpcost * who->p_ship->s_speed) / 2);
		who->p_ship->s_etemp += (who->p_ship->s_speed / 2);
	}
	/* Add in some for each army on board. */
	amt += ((who->p_ship->s_armies * who->p_ship->s_warpcost) / 2);
	who->p_ship->s_etemp += (who->p_ship->s_armies / 2);

	/* If they are towing someone, that costs too. */
	if (who->p_ship->s_flags & SFTOWING) {
		towee = &players[who->p_ship->s_towing];
		amt += ((towee->p_ship->s_warpcost * who->p_ship->s_speed) / 2);
		who->p_ship->s_etemp += (who->p_ship->s_speed / 2);

		/* Shields up cost 50% more */
		if (who->p_ship->s_flags & SFSHIELD) {
			amt += ((towee->p_ship->s_warpcost * who->p_ship->s_speed) / 2);
			who->p_ship->s_etemp += (who->p_ship->s_speed / 2);
		}
		/* Add in some for each army on board. */
		amt += ((towee->p_ship->s_armies * towee->p_ship->s_warpcost) / 2);
		who->p_ship->s_etemp += (towee->p_ship->s_armies / 2);
	}

	who->p_ship->s_fuel -= amt;

	if (who->p_ship->s_fuel <= 0) {
		who->p_ship->s_fuel = 0;
		tow_off(who);
		who->p_ship->s_desspeed = 0;
		cloak_off(who);
	}
}

changedir(sp)
struct player *sp;
{
    int ticks;

	if (isGod(sp)) {
		sp->p_ship->s_dir = sp->p_ship->s_desdir;
		sp->p_ship->s_subdir = 0;
		return;
	}
    if (sp->p_ship->s_speed == 0) {
	sp->p_ship->s_dir = sp->p_ship->s_desdir;
	sp->p_ship->s_subdir = 0;
    }
    else {
	sp->p_ship->s_subdir += sp->p_ship->s_turns / (1 << sp->p_ship->s_speed);
	ticks = sp->p_ship->s_subdir / 1000;
	if (ticks) {
	    if (ticks > dirdiff(sp->p_ship->s_dir, sp->p_ship->s_desdir))
		sp->p_ship->s_dir = sp->p_ship->s_desdir;
	    else if ((unsigned char) (sp->p_ship->s_dir - sp->p_ship->s_desdir) > 127)
		sp->p_ship->s_dir += ticks;
	    else
		sp->p_ship->s_dir -= ticks;
	    sp->p_ship->s_subdir %= 1000;
	}
    }
}

dirdiff(a, b)
unsigned char a, b;
{
  unsigned char diff = ABS(a - b);
  return(diff > 128 ? 256 - diff : diff);
}
