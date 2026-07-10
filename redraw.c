static char sccsid[] = "@(#)redraw.c	3.1";
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
#include <signal.h>
#include <math.h>
#include "defs.h"
#include "data.h"
#include "bitmaps.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

/* NOTE: This definition depends on MAXPLAYERS! */
static char *shipnos = "0123456789abcdef";

static short nplayers;
static int stlinecount = 1;

extern unsigned char	newcourse();	/* Down below... */

intrupt()
{
    register int		i;
    register struct player	*p;

    udcounter++;
    move();
    ++stlinecount;
}

redraw(j)
register struct player	*j;
{
    /* erase warning line if necessary */
    if ((j->p_warntimer <= udcounter) && (j->p_warncount > 0)) {
	XClearWindow(j->display, j->warnw);
	j->p_warncount = 0;
    }

    if (j->clearcount) {
	XFillRectangles(j->display, j->w, j->cleargc,
		j->clearzone, j->clearcount);
	j->clearcount = 0;
    }

/* NOTE: Can this be made into an XDrawLines? */
    while (j->clearlcount) {
	j->clearlcount--;
	XDrawLine(j->display, j->w, j->cleargc,
	    j->clearline[0][j->clearlcount],
	    j->clearline[1][j->clearlcount],
	    j->clearline[2][j->clearlcount],
	    j->clearline[3][j->clearlcount]);
    }

    if ((j->p_mapmode) && (udcounter % ((nplayers == 0) ? 1 : nplayers) == 0))
	map(j);

    /* Display a new message every MESSTIME/UPS seconds */
    if ((udcounter % MESSTIME) == 0)
	dmessage(j);

    local(j);	/* redraw local window */

    if ((stlinecount&02) == 0)
	stline(j);	/* update every 4 times we update the window */

    if (j->p_flags & PFSHOWSTATS)
	updateStats(j, j->statwin);
}

drawPartialPhaser(p, j)
register struct player *p, *j;
{
int	sx,ex,sy,ey;
struct	phaser *php;


    sx= j->p_ship->s_x - p->p_ship->s_x;
    sy= j->p_ship->s_y - p->p_ship->s_y;
    /* Now draw his phaser (if it exists) */
    php = &phasers[j->p_ship->s_no];
    if (php->ph_status == PHMISS) {
	/* Here I will have to compute end coordinate */
	phasedist(j, php, &ex, &ey);
	ex -= p->p_ship->s_x;
	ey -= p->p_ship->s_y;
    }
    else {
	ex = (players[php->ph_target].p_ship->s_x) - p->p_ship->s_x;
	ey = (players[php->ph_target].p_ship->s_y) - p->p_ship->s_y;
    }
    /* phaser end points are now relative to our position, make them
     * relative to window origin (so X clips them correctly)
     */
    sx= sx/SCALE+p->p_xwinsize/2; sy= sy/SCALE+p->p_ywinsize/2;
    ex= ex/SCALE+p->p_xwinsize/2; ey= ey/SCALE+p->p_ywinsize/2;
    if ((sx<0)&&(ex<0))	return;
    if ((sy<0)&&(ey<0))	return;
    if ((sx>p->p_xwinsize)&&(ex>p->p_xwinsize))	return;
    if ((sy>p->p_ywinsize)&&(ey>p->p_ywinsize))	return;
    XSetForeground(p->display, p->xfgc, phaserColor(php));
    XDrawLine(p->display, p->w, p->xfgc,
	    sx, sy, ex, ey);
/*
    p->clearline[p->clearlcount].x1 = sx;
    p->clearline[p->clearlcount].y1 = sy;
    p->clearline[p->clearlcount].x2 = ex;
    p->clearline[p->clearlcount].y2 = ey;
*/
    p->clearline[0][p->clearlcount] = sx;
    p->clearline[1][p->clearlcount] = sy;
    p->clearline[2][p->clearlcount] = ex;
    p->clearline[3][p->clearlcount] = ey;
    p->clearlcount++;
}

local(p)
register struct player *p;
{
    register int h, i;
    register struct player *j;
    register struct torp *k;
    register struct planet *l;
    register struct phaser *php;
    register struct ship	*towee;
    char glyph;
    int dx, dy;
    int xview, yview;

    /* Draw Planets */
    xview = SCALE * p->p_xwinsize / 2;
    yview = SCALE * p->p_ywinsize / 2;
    for (i = 0, l = &planets[0]; i < MAXPLANETS; i++, l++) {
	if (l->pl_flags & PLINVIS)
		continue;
	dx = l->pl_x - p->p_ship->s_x;
	dy = l->pl_y - p->p_ship->s_y;
	if (dx > xview || dx < -xview || dy > yview || dy < -yview)
	    continue;
	dx = dx / SCALE + p->p_xwinsize / 2;
	dy = dy / SCALE + p->p_ywinsize / 2;
	glyph = planetGlyph(l);
	XSetForeground(p->display, p->xfgc, planetColor(l));
	XDrawString(p->display, p->w, p->xfgc, dx - (planet_width/2),
		dy - (planet_height/2), &glyph, 1);
	if (p->p_namemode) {
	    XDrawImageString(p->display, p->w, p->dfgc, dx - (planet_width/2), dy + (planet_height/2 + p->dfont->ascent),
		l->pl_name, l->pl_namelen);
	    p->clearzone[p->clearcount].x = dx - (planet_width/2);
	    p->clearzone[p->clearcount].y = dy + (planet_height/2);
	    /*NOTE: put this calculation into the player data */
	    p->clearzone[p->clearcount].width = XTextWidth(p->dfont, l->pl_name, l->pl_namelen);
	    p->clearzone[p->clearcount].height = fontHeight(p->dfont);
	    p->clearcount++;
	}
	p->clearzone[p->clearcount].x = dx - (planet_width/2);
	p->clearzone[p->clearcount].y = dy - (planet_height/2);
	p->clearzone[p->clearcount].width = planet_width;
	p->clearzone[p->clearcount].height = planet_height;
	p->clearcount++;
    }

    /* Draw ships */
    nplayers = 0;
    for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	int tx, ty;
	if (j->p_status != PALIVE)
	    continue;
	if (j->p_ship->s_status == EXPLODE && j->p_ship->s_explode < 0)
		continue;
	/* Always draw self or if viewer is God */
	if (j != p && !isGod(p)) {
		/* Don't draw enemies or God if they are cloaked */
		if (iscloaked(j->p_ship) && (!friendlyPlayer(j) || isGod(j)))
			continue;
	}
	nplayers++;
	dx = j->p_ship->s_x - p->p_ship->s_x;
	dy = j->p_ship->s_y - p->p_ship->s_y;
	if (dx > xview || dx < -xview || dy > yview || dy < -yview) {
	    if (phasers[j->p_ship->s_no].ph_status != PHFREE)
		drawPartialPhaser(p, j);
	    continue;
	}
	XSetForeground(p->display, p->xfgc, playerColor(j));
	dx = dx / SCALE + p->p_xwinsize / 2;
	dy = dy / SCALE + p->p_ywinsize / 2;
	if (j->p_ship->s_status != EXPLODE) {
	    if (j->p_ship->s_team == FED)
		glyph = FED_GLYPHS + rosette(j->p_ship->s_dir);
	    else if (j->p_ship->s_team == ROM)
		glyph = ROM_GLYPHS + rosette(j->p_ship->s_dir);
	    else if (j->p_ship->s_team == KLI)
		glyph = KLI_GLYPHS + rosette(j->p_ship->s_dir);
	    else if (j->p_ship->s_team == ORI)
		glyph = ORI_GLYPHS + rosette(j->p_ship->s_dir);
	    else if (j->p_ship->s_team == GOD)
		glyph = GODVIEW;

	    XDrawString(p->display, p->w, p->xfgc,
	        dx - (ship_width/2), dy - (ship_height/2),
		&glyph, 1);
	    if (iscloaked(j->p_ship))
		XFillRectangle(p->display, p->w, p->dimgc,
	            dx - (ship_width/2), dy - (ship_height/2),
		    ship_width, ship_height);
	/* NOTE: This DrawImageString doesn't use the right GC */
	    if (j->p_ship->s_team != GOD)
		    XDrawImageString(p->display, p->w, p->dfgc, dx + (ship_width/2), dy - (ship_height/2 - p->dfont->ascent),
			shipnos + j->p_ship->s_no, 1);

/* NOTE: fix to change shield color depending on damage.. */
	    if ((p->p_flags & PFSHOWSHIELDS) && (j->p_ship->s_flags & SFSHIELD)) {
		/* Don't draw shields around God's "ship". */
		if (j->p_ship->s_team != GOD) {
			if (j == p) {
			    if (j->p_ship->s_damage > (j->p_ship->s_maxdamage / 2))
				glyph = RSHIELD_GLYPH;
			    else if (j->p_ship->s_shield < (j->p_ship->s_maxshields / 2))
				glyph = YSHIELD_GLYPH;
			    else
				glyph = SHIELD_GLYPH;
			} else
			    glyph = SHIELD_GLYPH;
			XDrawString(p->display, p->w, p->xfgc, dx - (shield_width / 2),
				dy - (shield_height / 2), &glyph, 1);
		}
	    }

	    p->clearzone[p->clearcount].x = dx + (ship_width/2);
	    p->clearzone[p->clearcount].y = dy - (ship_height/2);
	    /* NOTE: put this calculattion into  player data */
	    p->clearzone[p->clearcount].width = XTextWidth(p->dfont, shipnos + j->p_ship->s_no, 1);
	    p->clearzone[p->clearcount].height = fontHeight(p->dfont);
	    p->clearcount++;
	    p->clearzone[p->clearcount].x = dx - (shield_width/2);
	    p->clearzone[p->clearcount].y = dy - (shield_height/2);
	    p->clearzone[p->clearcount].width = shield_width;
	    p->clearzone[p->clearcount].height = shield_height;
	    p->clearcount++;
	} else {
	    glyph = EXP_GLYPHS_LEFT + (10 - j->p_ship->s_explode)/2;
	    XDrawString(p->display, p->w, p->xfgc, dx - (ex_width / 2),
		dy - (ex_height / 2), &glyph, 1);

	    glyph = EXP_GLYPHS_RIGHT + (10 - j->p_ship->s_explode)/2;
	    XDrawString(p->display, p->w, p->xfgc, dx,
		dy - (ex_height / 2), &glyph, 1);

	    p->clearzone[p->clearcount].x = dx - (ex_width/2);
	    p->clearzone[p->clearcount].y = dy - (ex_height/2);
	    p->clearzone[p->clearcount].width = ex_width;
	    p->clearzone[p->clearcount].height = ex_height;
	    p->clearcount++;
	}

	/* Now draw their phaser (if it exists) */
	php = &phasers[j->p_ship->s_no];
	if (php->ph_status != PHFREE) {
	    if (php->ph_status == PHMISS) {
		/* Here I will have to compute end coordinate */
		phasedist(j, php, &tx, &ty);
		tx = (tx - p->p_ship->s_x) / SCALE + p->p_xwinsize / 2;
		ty = (ty - p->p_ship->s_y) / SCALE + p->p_ywinsize / 2;
		XSetForeground(p->display, p->xfgc, phaserColor(php));
		XDrawLine(p->display, p->w, p->xfgc, dx, dy, tx, ty);
	    }
	    else { /* Start point is dx, dy */
		tx = (players[php->ph_target].p_ship->s_x - p->p_ship->s_x) /
		    SCALE + p->p_xwinsize / 2;
		ty = (players[php->ph_target].p_ship->s_y - p->p_ship->s_y) /
		    SCALE + p->p_ywinsize / 2;
		XSetForeground(p->display, p->xfgc, phaserColor(php));
		XDrawLine(p->display, p->w, p->xfgc, dx, dy, tx, ty);
	    }
	    p->clearline[0][p->clearlcount] = dx;
	    p->clearline[1][p->clearlcount] = dy;
	    p->clearline[2][p->clearlcount] = tx;
	    p->clearline[3][p->clearlcount] = ty;
	    p->clearlcount++;
	}

	/* Now, if they are towing someone...draw the tractor beam. */
	if (j->p_ship->s_flags & SFTOWING) {
	    towee = &ships[j->p_ship->s_towing];
	    tx = (towee->s_x - p->p_ship->s_x) /
		SCALE + p->p_xwinsize / 2;
	    ty = (towee->s_y - p->p_ship->s_y) /
		SCALE + p->p_ywinsize / 2;
	    XSetForeground(p->display, p->tbgc, p->unColor);
	    XDrawLine(p->display, p->w, p->tbgc, dx, dy, tx, ty);
	    p->clearline[0][p->clearlcount] = dx;
	    p->clearline[1][p->clearlcount] = dy;
	    p->clearline[2][p->clearlcount] = tx;
	    p->clearline[3][p->clearlcount] = ty;
	    p->clearlcount++;
	}
    }

    /* Draw torps */
    for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	if (j->p_status != PALIVE)
		continue;
	if (!j->p_ship->s_ntorp)
	    continue;
	for (h = 0, k = &torps[MAXTORP * i + h]; h < MAXTORP; h++, k++) {
	    if (!k->t_status)
		continue;
	    dx = k->t_x - p->p_ship->s_x;
	    dy = k->t_y - p->p_ship->s_y;
	    if (dx > xview || dx < -xview || dy > yview || dy < -yview)
		continue;
	    XSetForeground(p->display, p->xfgc, torpColor(k));
	    dx = dx / SCALE + p->p_xwinsize / 2;
	    dy = dy / SCALE + p->p_ywinsize / 2;
	    if (k->t_status == TEXPLODE) {
		glyph = CLOUD_GLYPH;
		XDrawString(p->display, p->w, p->xfgc, dx - (cloud_width / 2),
			dy - (cloud_height / 2), &glyph, 1);

		p->clearzone[p->clearcount].x = dx - (cloud_width/2);
		p->clearzone[p->clearcount].y = dy - (cloud_height/2);
		p->clearzone[p->clearcount].width = cloud_width;
		p->clearzone[p->clearcount].height = cloud_height;
		p->clearcount++;
	    }
	    else if (k->t_owner != p->p_ship->s_no && ((k->t_war & (1 << p->p_ship->s_team)) ||
		      ((1 << k->t_team) & (p->p_ship->s_hostile | p->p_ship->s_swar))))
	    {
		glyph = ETORP_GLYPH;
		XDrawString(p->display, p->w, p->xfgc, dx - (etorp_width / 2),
			dy - (etorp_height / 2), &glyph, 1);

		p->clearzone[p->clearcount].x = dx - (etorp_width/2);
		p->clearzone[p->clearcount].y = dy - (etorp_height/2);
		p->clearzone[p->clearcount].width = etorp_width;
		p->clearzone[p->clearcount].height = etorp_height;
		p->clearcount++;
	    }
	    else {
		glyph = MTORP_GLYPH;
		XDrawString(p->display, p->w, p->xfgc, dx - (mtorp_width / 2),
			dy - (mtorp_height / 2), &glyph, 1);

		p->clearzone[p->clearcount].x = dx - (mtorp_width/2);
		p->clearzone[p->clearcount].y = dy - (mtorp_height/2);
		p->clearzone[p->clearcount].width = mtorp_width;
		p->clearzone[p->clearcount].height = mtorp_height;
		p->clearcount++;
	    }
	}
    }

    /* Draw Edges */
    XSetForeground(p->display, p->xfgc, p->warningColor);
    if (p->p_ship->s_x < (p->p_xwinsize / 2) * SCALE) {
	int	sy, ey;

	dx = (p->p_xwinsize / 2) - (p->p_ship->s_x) / SCALE;
	sy = (p->p_ywinsize / 2) + (0 - p->p_ship->s_y) / SCALE;
	ey = (p->p_ywinsize / 2) + (GWIDTH - p->p_ship->s_y) / SCALE;
	if (sy < 0) sy = 0;
	if (ey > p->p_ywinsize - 1) ey = p->p_ywinsize - 1;
	XDrawLine(p->display, p->w, p->xfgc, dx, sy, dx, ey);
	p->clearline[0][p->clearlcount] = dx;
	p->clearline[1][p->clearlcount] = sy;
	p->clearline[2][p->clearlcount] = dx;
	p->clearline[3][p->clearlcount] = ey;
	p->clearlcount++;
    }

    if ((GWIDTH - p->p_ship->s_x) < (p->p_xwinsize / 2) * SCALE) {
	int	sy, ey;

	dx = (p->p_xwinsize / 2) + (GWIDTH - p->p_ship->s_x) / SCALE;
	sy = (p->p_ywinsize / 2) + (0 - p->p_ship->s_y) / SCALE;
	ey = (p->p_ywinsize / 2) + (GWIDTH - p->p_ship->s_y) / SCALE;
	if (sy < 0) sy = 0;
	if (ey > p->p_ywinsize - 1) ey = p->p_ywinsize - 1;
	XDrawLine(p->display, p->w, p->xfgc, dx, sy, dx, ey);
	p->clearline[0][p->clearlcount] = dx;
	p->clearline[1][p->clearlcount] = sy;
	p->clearline[2][p->clearlcount] = dx;
	p->clearline[3][p->clearlcount] = ey;
	p->clearlcount++;
    }

    if (p->p_ship->s_y < (p->p_ywinsize / 2) * SCALE) {
	int	sx, ex;

	dy = (p->p_ywinsize / 2) - (p->p_ship->s_y) / SCALE;
	sx = (p->p_xwinsize / 2) + (0 - p->p_ship->s_x) / SCALE;
	ex = (p->p_xwinsize / 2) + (GWIDTH - p->p_ship->s_x) / SCALE;
	if (sx < 0) sx = 0;
	if (ex > p->p_xwinsize - 1) ex = p->p_xwinsize - 1;
	XDrawLine(p->display, p->w, p->xfgc, sx, dy, ex, dy);
	p->clearline[0][p->clearlcount] = sx;
	p->clearline[1][p->clearlcount] = dy;
	p->clearline[2][p->clearlcount] = ex;
	p->clearline[3][p->clearlcount] = dy;
	p->clearlcount++;
    }

    if ((GWIDTH - p->p_ship->s_y) < (p->p_ywinsize / 2) * SCALE) {
	int	sx, ex;

	dy = (p->p_ywinsize / 2) + (GWIDTH - p->p_ship->s_y) / SCALE;
	sx = (p->p_xwinsize / 2) + (0 - p->p_ship->s_x) / SCALE;
	ex = (p->p_xwinsize / 2) + (GWIDTH - p->p_ship->s_x) / SCALE;
	if (sx < 0) sx = 0;
	if (ex > p->p_xwinsize - 1) ex = p->p_xwinsize - 1;
	XDrawLine(p->display, p->w, p->xfgc, sx, dy, ex, dy);
	p->clearline[0][p->clearlcount] = sx;
	p->clearline[1][p->clearlcount] = dy;
	p->clearline[2][p->clearlcount] = ex;
	p->clearline[3][p->clearlcount] = dy;
	p->clearlcount++;
    }

    /* Change border color to signify alert status */
    if (p->p_ship->s_oldalert != (p->p_ship->s_flags & (SFGREEN|SFYELLOW|SFRED))) {
        p->p_ship->s_oldalert = (p->p_ship->s_flags & (SFGREEN|SFYELLOW|SFRED));
	switch (p->p_ship->s_oldalert) {
	    case SFGREEN:
	        if (!p->mono) {
		    XSetWindowBorder(p->display, p->baseWin, p->gColor);
/*ICCCM 4.1.9	    XSetWindowBorder(p->display, p->iconWin, p->gColor); */
	        } else {
		    XSetWindowBorderPixmap(p->display, p->baseWin, p->gTile);
/*ICCCM 4.1.9	    XSetWindowBorderPixmap(p->display, p->iconWin, p->gTile); */
		}
		break;
	    case SFYELLOW:
	        if (!p->mono) {
		    XSetWindowBorder(p->display, p->baseWin, p->yColor);
/*ICCCM 4.1.9	    XSetWindowBorder(p->display, p->iconWin, p->yColor); */
	        } else {
		    XSetWindowBorderPixmap(p->display, p->baseWin, p->yTile);
/*ICCCM 4.1.9	    XSetWindowBorderPixmap(p->display, p->iconWin, p->yTile); */
		}
		break;
	    case SFRED:
	        if (!p->mono) {
		    XSetWindowBorder(p->display, p->baseWin, p->rColor);
/*ICCCM 4.1.9	    XSetWindowBorder(p->display, p->iconWin, p->rColor); */
	        } else {
		    XSetWindowBorderPixmap(p->display, p->baseWin, p->rTile);
/*ICCCM 4.1.9	    XSetWindowBorderPixmap(p->display, p->iconWin, p->rTile); */
		}
		break;
	}
    }
}

/*
 * compute position of the end of the phaser
 * and return in tx and ty.
 */
phasedist(j, php, tx, ty)
    struct player *j;
    struct phaser *php;
    int *tx, *ty;
{
	*tx = j->p_ship->s_x + (int) (((j->p_ship->s_phasedist) * icos[php->ph_dir]) >> TRIGSCALE);
	*ty = j->p_ship->s_y + (int) (((j->p_ship->s_phasedist) * isin[php->ph_dir]) >> TRIGSCALE);
}

map(p)
register struct player	*p;
{
    register int		i;
    register struct player	*j;
    register struct planet	*l;
    int				dx, dy;
    char			glyph;

    if (p->p_redrawall) {
	XClearWindow(p->display, p->mapw);
	p->mclearcount = 0;
    } else if (p->mclearcount) {
	XFillRectangles(p->display, p->mapw, p->cleargc,
		p->mclearzone, p->mclearcount);
	p->mclearcount = 0;
    }

    /* Draw Planets */
    for (i = 0, l = &planets[0]; i < MAXPLANETS; i++, l++) {
	if (l->pl_flags & PLINVIS)
		continue;
	if ((l->pl_drawtime < p->p_ship->s_updates) && (!p->p_redrawall))
	    continue;
	dx = l->pl_x * p->p_xwinsize / GWIDTH;
	dy = l->pl_y * p->p_ywinsize / GWIDTH;

	if (l->pl_orbvel && p->mpzset[l->pl_no] && !p->p_redrawall) {
		/* Erase its last position... */
		XFillRectangles(p->display, p->mapw, p->cleargc,
			&p->mpzone[l->pl_no * 2], 2);
		p->mpzset[l->pl_no] = 0;
	}
	glyph = mplanetGlyph(l);
	XSetForeground(p->display, p->xfgc, planetColor(l));
	XDrawString(p->display, p->mapw, p->xfgc, dx - (mplanet_width / 2),
		dy - (mplanet_height / 2), &glyph, 1);
	XDrawImageString(p->display, p->mapw, p->dfgc, dx - (mplanet_width/2), dy + (mplanet_height/2 + p->dfont->ascent),
	    l->pl_name, 3);

	if (l->pl_orbvel) {
		/* Save its present position... */
		p->mpzone[l->pl_no * 2].x = dx - (mplanet_width/2);
		p->mpzone[l->pl_no * 2].y = dy + (mplanet_height/2);
		/* NOTE: Put this calculation into player data */
		p->mpzone[l->pl_no * 2].width = XTextWidth(p->dfont, l->pl_name, 3);
		p->mpzone[l->pl_no * 2].height = fontHeight(p->dfont);
		p->mpzone[l->pl_no * 2 + 1].x = dx - (mplanet_width/2);
		p->mpzone[l->pl_no * 2 + 1].y = dy - (mplanet_height/2);
		p->mpzone[l->pl_no * 2 + 1].width = mplanet_width;
		p->mpzone[l->pl_no * 2 + 1].height = mplanet_height;
		p->mpzset[l->pl_no] = 1;
	}
    }
    p->p_redrawall = 0;

    /* Draw ships */
    nplayers = 0;
    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	char	*str;

	if (j->p_status != PALIVE)
	    continue;
	/* NOTE: This counter includes ALL team members. */
	nplayers++;
	dx = j->p_ship->s_x * p->p_xwinsize / GWIDTH;
	dy = j->p_ship->s_y * p->p_ywinsize / GWIDTH;
	if (isGod(p) || !iscloaked(j->p_ship) || friendlyPlayer(j)) {
		/* NOTE: put these calculations into the player data */
	    XDrawImageString(p->display, p->mapw, p->dfgc, dx - XTextWidth(p->dfont, j->p_mapchars, 2), dy - fontHeight(p->dfont)/2 + p->dfont->ascent,
		str = j->p_mapchars, 2);
	    p->mclearzone[p->mclearcount].x = dx - XTextWidth(p->dfont, j->p_mapchars, 2);
	    p->mclearzone[p->mclearcount].y = dy - fontHeight(p->dfont)/2;
	    p->mclearzone[p->mclearcount].width = XTextWidth(p->dfont, str, 2);
	    p->mclearzone[p->mclearcount].height = fontHeight(p->dfont);
	    p->mclearcount++;
	}
    }
}

stline(p)
register struct player	*p;
{
/*                         0123456789012345678901234567890123456789012345678901234567890*/
    static char buf[80] = "               0     0 100   0    0.00    0  10000    0     0";

    /* Instead of one sprintf, we do all this by hand for optimization */

    if (buf[0] == 0) {
	int i;
	for (i = 0; i < 63; ++i)
	    buf[i] = ' ';
    }

    buf[0] = (p->p_ship->s_flags & SFSHIELD ? 'S': ' ');
    if (p->p_ship->s_flags & SFGREEN)
	buf[1] = 'G';
    else if (p->p_ship->s_flags & SFYELLOW)
	buf[1] = 'Y';
    else if (p->p_ship->s_flags & SFRED)
	buf[1] = 'R';
    buf[2] = (p->p_ship->s_flags & (SFPLLOCK | SFSLOCK) ? 'L': ' ');
    buf[3] = (p->p_ship->s_flags & SFREPAIR ? 'R': ' ');
    buf[4] = (p->p_ship->s_flags & SFBOMB ? 'B': ' ');
    buf[5] = (p->p_ship->s_flags & SFORBIT ? 'O': ' ');
    if ((p->p_ship->s_flags & SFCLOAK) && !(p->p_ship->s_flags & SFDCLOAK))
	buf[6] = 'C';
    else
	buf[6] = ' ';
    buf[7] = (p->p_ship->s_flags & SFWEP ? 'W': ' ');
    buf[8] = (p->p_ship->s_flags & SFENG ? 'E': ' ');
    buf[9] = (p->p_ship->s_flags & SFBEAMUP ? 'u': ' ');
    buf[10] = (p->p_ship->s_flags & SFBEAMDOWN ? 'd': ' ');
    buf[11] = (p->p_flags & PFCOPILOT ? 'P' : ' ');
    buf[14] = '0' + (p->p_ship->s_speed / 10);	/* speed */
    if (buf[14] == '0')
	buf[14] = ' ';
    buf[15] = '0' + (p->p_ship->s_speed % 10);	/* speed */
    buf[19] = '0' + (p->p_ship->s_damage / 100);
    if (buf[19] == '0')
	buf[19] = ' ';
    buf[20] = '0' + ((p->p_ship->s_damage % 100) / 10);
    if ((buf[20] == '0') && (p->p_ship->s_damage < 100))
	buf[20] = ' ';
    buf[21] = '0' + (p->p_ship->s_damage % 10);
    buf[23] = '0' + (p->p_ship->s_shield / 100);
    if (buf[23] == '0')
	buf[23] = ' ';
    buf[24] = '0' + ((p->p_ship->s_shield % 100) / 10);
    if ((buf[24] == '0') && (p->p_ship->s_shield < 100))
	buf[24] = ' ';
    buf[25] = '0' + (p->p_ship->s_shield % 10);
    buf[28] = '0' + ((p->p_ship->s_ntorp % 100) / 10);
    if (buf[28] == '0')
	buf[28] = ' ';
    buf[29] = '0' + (p->p_ship->s_ntorp % 10);
    buf[33] = '0' + ((int) (p->p_ship->s_stats.st_kills / 10));
    if (buf[33] == '0')
	buf[33] = ' ';
    buf[34] = '0' + (((int) p->p_ship->s_stats.st_kills) % 10);
    buf[35] = '.';
    buf[36] = '0' + (((int) (p->p_ship->s_stats.st_kills * 10)) % 10);
    buf[37] = '0' + (((int) (p->p_ship->s_stats.st_kills * 100)) % 10);
    buf[41] = '0' + ((p->p_ship->s_armies % 100) / 10);
    if (buf[41] == '0')
	buf[41] = ' ';
    buf[42] = '0' + (p->p_ship->s_armies % 10);

    buf[45] = '0' + (p->p_ship->s_fuel / 10000);
    if (buf[45] == '0')
	buf[45] = ' ';
    buf[46] = '0' + ((p->p_ship->s_fuel % 10000) / 1000);
    if ((buf[46] == '0') && (p->p_ship->s_fuel < 10000))
	buf[46] = ' ';
    buf[47] = '0' + ((p->p_ship->s_fuel % 1000) / 100);
    if ((buf[47] == '0') && (p->p_ship->s_fuel < 1000))
	buf[47] = ' ';
    buf[48] = '0' + ((p->p_ship->s_fuel % 100) / 10);
    if ((buf[48] == '0') && (p->p_ship->s_fuel < 100))
	buf[48] = ' ';
    buf[49] = '0' + (p->p_ship->s_fuel % 10);

    buf[52] = '0' + ((p->p_ship->s_wtemp / 10) / 100);
    if (buf[52] == '0')
	buf[52] = ' ';
    buf[53] = '0' + (((p->p_ship->s_wtemp / 10) % 100) / 10);
    if ((buf[53] == '0') && (p->p_ship->s_wtemp < 1000))
	buf[53] = ' ';
    buf[54] = '0' + ((p->p_ship->s_wtemp / 10) % 10);

    buf[58] = '0' + ((p->p_ship->s_etemp / 10) / 100);
    if (buf[58] == '0')
	buf[58] = ' ';
    buf[59] = '0' + (((p->p_ship->s_etemp / 10) % 100) / 10);
    if ((buf[59] == '0') && (p->p_ship->s_etemp < 1000))
	buf[59] = ' ';
    buf[60] = '0' + ((p->p_ship->s_etemp / 10) % 10);
    buf[61] = '\0';

    /* Draw status line */
    if (!p->p_ts_offset)
	redrawTstats(p);
    XDrawImageString(p->display, p->tstatw, p->dfgc, p->p_ts_offset, 20 + p->dfont->ascent, buf, 61);

#ifdef notdef

    /* This code is being left around because it is much more elegant
    ** than that above.  However, it lacks a tremendous amount in efficiency.
    */
    char alertchar = '?';

    if (me->p_flags & SFGREEN)
	alertchar = 'G';
    else if (me->p_flags & SFYELLOW)
	alertchar = 'Y';
    else if (me->p_flags & SFRED)
	alertchar = 'R';
    /* Draw status line */
    sprintf(buf,
"%c%c%c%c%c%c%c%c%c%c%c%c   %1d  %3d %3d  %2d    %5.2f  %2d    %5d   %3d   %3d",
	(me->p_flags & SFSHIELD ? 'S': ' '),
	alertchar,
	(me->p_flags & (SFPLLOCK | SFSLOCK) ? 'L': ' '),
	(me->p_flags & SFREPAIR ? 'R': ' '),
	(me->p_flags & SFBOMB ? 'B': ' '),
	(me->p_flags & SFORBIT ? 'O': ' '),
	(me->p_flags & SFCLOAK ? 'C': ' '),
	(me->p_flags & SFWEP ? 'W': ' '),
	(me->p_flags & SFENG ? 'E': ' '),
	(me->p_flags & SFBEAMUP ? 'u': ' '),
	(me->p_flags & SFBEAMDOWN ? 'd': ' '),
	(me->p_flags & PFCOPILOT ? 'P' : ' '),
	me->p_speed,
	me->p_damage,
	me->p_shield,
	me->p_ntorp,
	me->p_ship->s_stats.st_kills,
	me->p_armies,
	me->p_fuel,
	me->p_wtemp/10,
	me->p_etemp/10);
    XDrawImageString(display, tstatw, dfgc, 50, 20, buf, strlen(buf));

#endif notdef
}

/* These are routines that need to be done on interrupts but
   don't belong in the redraw routine and particularly don't
   belong in the daemon. */

auto_features(p)
register struct player	*p;
{
    register int		i;
    char buf[80];
    struct player *pl;
    struct planet *pln;

    if (p->p_ship->s_flags & SFSELFDEST) {
#ifdef notdef
	if ((p->p_ship->s_updates >= p->p_ship->s_selfdest) ||
	    ((p->p_flags & SFGREEN) && (p->p_damage == 0)
		&& (p->p_shield == 100)))
#else
	if (p->p_ship->s_updates >= p->p_ship->s_selfdest)
#endif
	{
	    p->p_ship->s_flags &= ~SFSELFDEST;
	    p->p_ship->s_explode = PEXPTIME;
	    p->p_ship->s_whydead = KQUIT;
	    p->p_ship->s_status = EXPLODE;
	}
	else {
	    sprintf(buf, "Self Destruct in %d seconds",
		(p->p_ship->s_selfdest - p->p_ship->s_updates) / UPS);
	    warning(p, buf);
	}
    }
    /* give certain information about bombing or beaming */
    if (p->p_ship->s_flags & SFBOMB) {
	if (planets[p->p_ship->s_planet].pl_armies < 5) {
	    sprintf(buf, "Cannot bomb %s while armies are less than 5",
		planets[p->p_ship->s_planet].pl_name);
	    warning(p, buf);
	    p->p_ship->s_flags &= ~SFBOMB;
	}
	else {
	    sprintf(buf, "Bombing %s.  %d armies left",
		planets[p->p_ship->s_planet].pl_name,
		planets[p->p_ship->s_planet].pl_armies);
	    warning(p, buf);
	}
    }

    if (p->p_ship->s_flags & SFBEAMUP) {
	if (planets[p->p_ship->s_planet].pl_armies < 5) {
	    sprintf(buf, "%s: Too few armies to beam up",
		planets[p->p_ship->s_planet].pl_name);
	    warning(p, buf);
	    p->p_ship->s_flags &= ~SFBEAMUP;
	}
	else if ((p->p_ship->s_armies == (int) (p->p_ship->s_stats.st_kills * 2)) ||
	    (p->p_ship->s_armies == p->p_ship->s_maxarmies)) {
		sprintf(buf, "No more room on board for armies");
	    warning(p, buf);
	    p->p_ship->s_flags &= ~SFBEAMUP;
	}
	else {
	    sprintf(buf, "Beaming up.  (%d/%d)", p->p_ship->s_armies,
		((p->p_ship->s_stats.st_kills * 2) > p->p_ship->s_maxarmies) ?
		    p->p_ship->s_maxarmies : (int) (p->p_ship->s_stats.st_kills * 2));
	    warning(p, buf);
	}
    }
    if (p->p_ship->s_flags & SFBEAMDOWN) {
	if (p->p_ship->s_armies == 0) {
	    sprintf(buf, "No more armies to beam down to %s.",
		planets[p->p_ship->s_planet].pl_name);
	    warning(p, buf);
	    p->p_ship->s_flags &= ~SFBEAMDOWN;
	}
	else {
	    sprintf(buf, "Beaming down.  (%d/%d) %s has %d armies left",
		p->p_ship->s_armies,
		((p->p_ship->s_stats.st_kills * 2) > p->p_ship->s_maxarmies) ?
		    p->p_ship->s_maxarmies : (int) (p->p_ship->s_stats.st_kills * 2),
		planets[p->p_ship->s_planet].pl_name,
		planets[p->p_ship->s_planet].pl_armies);
	    warning(p, buf);
	}
    }
    if (p->p_ship->s_flags & SFREPAIR) {
	if ((p->p_ship->s_damage == 0) && (p->p_ship->s_shield == p->p_ship->s_maxshields))
		p->p_ship->s_flags &= ~SFREPAIR;
    }
    if (p->p_ship->s_flags & SFSLOCK) { 	/* set course to player x */
	pl = &players[p->p_ship->s_shipl];
	if (pl->p_status != PALIVE)
	    p->p_ship->s_flags &= ~SFSLOCK;
	if (isGod(p)) {
		p->p_ship->s_x = pl->p_ship->s_x;
		p->p_ship->s_y = pl->p_ship->s_y;
	} else {
		/* Locks on cloaked ships can fail.. */
		if ((pl->p_ship->s_flags & SFCLOAK) && !(pl->p_ship->s_flags & SFDCLOAK)) {
			if ((random() % 100) >= (pl->p_ship->s_damage * 2)) {
				p->p_ship->s_flags &= ~SFSLOCK;
			}
		}
	}
	set_course(p, newcourse(p, pl->p_ship->s_x, pl->p_ship->s_y));
    }
    if (p->p_ship->s_flags & SFPLLOCK) { 	/* set course to planet x */
	int dist;
	pln = &planets[p->p_ship->s_planet];
	if (isGod(p)) {
		p->p_ship->s_x = pln->pl_x;
		p->p_ship->s_y = pln->pl_y;
		return;
	}
	dist = hypot((double) (p->p_ship->s_x - pln->pl_x),
	    (double) (p->p_ship->s_y - pln->pl_y));

	/* This is magic.  It should be based on defines, but it slows
	   the ship down to warp two an appropriate distance from the
	   planet for orbit */

	if (dist < (50 * ((p->p_ship->s_speed * (p->p_ship->s_speed+1)) + 10)))
	    set_speed(p, ORBSPEED);
	if ((dist < ORBDIST) && (p->p_ship->s_speed <= ORBSPEED))  {
	    p->p_ship->s_flags &= ~SFPLLOCK;
	    orbit(p);
	}
	else {
	    set_course(p, newcourse(p, pln->pl_x, pln->pl_y));
	}
    }
}

unsigned char
newcourse(p, x, y)
register struct player	*p;
register int		x, y;
{
	unsigned char	iatan2();

	return(iatan2(x - p->p_ship->s_x, p->p_ship->s_y - y));
}

redrawTstats(p)
register struct player	*p;
{
    char	buf[BUFSIZ];

    sprintf(buf,
	"Flags        warp  dam shd torps kills armies fuel wtemp etemp");
    if (!p->p_ts_offset)
	p->p_ts_offset = (p->p_xwinsize - XTextWidth(p->dfont, buf, strlen(buf))) / 2;
    XDrawImageString(p->display, p->tstatw, p->dfgc, p->p_ts_offset, 10 + p->dfont->ascent, buf, strlen(buf));
}
