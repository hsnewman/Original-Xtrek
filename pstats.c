static char sccsid[] = "@(#)pstats.c	3.1";

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

#if defined(hpux) || defined(cray)
#include <time.h>
#include <fcntl.h>
#else
#include <sys/time.h>
#endif

#include <sys/resource.h>
#include <sys/file.h>
#if defined(NDBM)
# include <ndbm.h>
#endif
#if !defined(NDBM)
#if !defined(cray)
#  include <dbm.h>
#else
#  include <rpcsvc/dbm.h>
#endif
#endif

#include "defs.h"
#include "data.h"

savestats(p)
register struct player	*p;
{
#if defined(NDBM)
    register DBM	*db;
#endif
    char		buf[80];
    datum		key, sinfo;
    long		now;
    struct stats	old;
    char		fname[132];

	sprintf(fname, "%s/%s", DIR, SCOREFILE);

#if defined(NDBM)
    db = dbm_open(fname, O_RDWR, 0700);
    if (db == (DBM *) NULL)
	return;
#else
    dbminit(fname);
#endif

    if (p->p_flags & PFROBOT)
	sprintf(buf, "%s", p->p_name);
    else
	sprintf(buf, "%s", p->p_login);

    time(&now);
    p->p_stats.st_time += (now - p->p_start_time);

    if (p->p_stats.st_maxkills < p->p_ship->s_stats.st_kills)
	p->p_stats.st_maxkills = p->p_ship->s_stats.st_kills;

    sinfo.dptr = (char *) &p->p_stats;
    sinfo.dsize = sizeof (struct stats);
    key.dptr = buf;
    key.dsize = strlen(buf) + 1;
#if defined(NDBM)
    dbm_store(db, key, sinfo, DBM_INSERT);
    (void) dbm_close(db);
#else
    store(key, sinfo);
    dbmclose();
#endif
}

getstats(p)
register struct player	*p;
{
#if defined(NDBM)
    register DBM	*db;
#endif
    char		buf[80];
    datum		key, sinfo;
    char		fname[132];

	sprintf(fname, "%s/%s", DIR, SCOREFILE);

#if defined(NDBM)
    db = dbm_open(fname, O_RDWR, 0700);
    if (db == (DBM *) NULL) {
	resetpstats(p);
	return;
    }
#else
    dbminit(fname);
#endif

    if (p->p_flags & PFROBOT)
	sprintf(buf, "%s", p->p_name);
    else
	sprintf(buf, "%s", p->p_login);

    key.dptr = buf;
    key.dsize = strlen(buf) + 1;
#if defined(NDBM)
    sinfo = dbm_fetch(db, key);
#else
    sinfo = fetch(key);
#endif
    if (sinfo.dptr) {
	bcopy(sinfo.dptr, &p->p_stats, sizeof (struct stats));
        key.dptr = buf;
        key.dsize = strlen(buf) + 1;
#if defined(NDBM)
	dbm_delete(db, key);
#else
	delete(key);
#endif
    } else
	resetpstats(p);

#if defined(NDBM)
    (void) dbm_close(db);
#else
    dbmclose();
#endif
}

resetsstats(p)
register struct player	*p;
{
	p->p_ship->s_stats.st_kills = 0;
	p->p_ship->s_stats.st_losses = 0;
	p->p_ship->s_stats.st_entries = 0;
	p->p_ship->s_stats.st_conqs = 0;
	p->p_ship->s_stats.st_coups = 0;
	p->p_ship->s_stats.st_torps = 0;
	p->p_ship->s_stats.st_phasers = 0;
	p->p_ship->s_stats.st_armsbomb = 0;
	p->p_ship->s_stats.st_armsship = 0;
	p->p_ship->s_stats.st_planets = 0;
	p->p_ship->s_stats.st_genocides = 0;
}

resetpstats(p)
register struct player	*p;
{
	p->p_stats.st_kills = 0;
	p->p_stats.st_losses = 0;
	p->p_stats.st_entries = 0;
	p->p_stats.st_conqs = 0;
	p->p_stats.st_coups = 0;
	p->p_stats.st_torps = 0;
	p->p_stats.st_phasers = 0;
	p->p_stats.st_armsbomb = 0;
	p->p_stats.st_armsship = 0;
	p->p_stats.st_planets = 0;
	p->p_stats.st_genocides = 0;
}

calcstats(p)
register struct player	*p;
{
	p->p_stats.st_kills += p->p_ship->s_stats.st_kills;
	if (p->p_stats.st_maxkills < p->p_ship->s_stats.st_kills)
	    p->p_stats.st_maxkills = p->p_ship->s_stats.st_kills;

	p->p_stats.st_losses += p->p_ship->s_stats.st_losses;
	p->p_stats.st_entries += p->p_ship->s_stats.st_entries;
	p->p_stats.st_conqs += p->p_ship->s_stats.st_conqs;
	p->p_stats.st_coups += p->p_ship->s_stats.st_coups;
	p->p_stats.st_torps += p->p_ship->s_stats.st_torps;
	p->p_stats.st_phasers += p->p_ship->s_stats.st_phasers;
	p->p_stats.st_armsbomb += p->p_ship->s_stats.st_armsbomb;
	p->p_stats.st_armsship += p->p_ship->s_stats.st_armsship;
	p->p_stats.st_planets += p->p_ship->s_stats.st_planets;
	p->p_stats.st_genocides += p->p_ship->s_stats.st_genocides;
}

void
liststats(fid)
int	fid;
{
#if defined(NDBM)
    DBM		*sdb;
#endif
    datum	key, sinfo;
    char	buf[132];
    char	fname[132];
    struct stats old;
    extern int	errno;

	sprintf(fname, "%s/%s", DIR, SCOREFILE);

#if defined(NDBM)
    sdb = dbm_open(fname, O_RDONLY, 0700);
    if (sdb == (DBM *) NULL)
#else
    if (dbminit(fname) < 0)
#endif
    {
	sprintf(buf, "liststats: Can't open SCOREFILE %s\nerror = %d\n", fname, errno);
	write(fid, buf, strlen(buf));
	return;
    }

  /*         0123456789012345678901234567890123456789012345678901234567890*/
strcpy(buf, "Name                     rtime     kills  maxkills  losses\n");
	write(fid, buf, strlen(buf));
strcpy(buf, "entries conqs coups   torps phasers  abomb aship planets gens ratio\n\n");
	write(fid, buf, strlen(buf));

#if defined(NDBM)
    key = dbm_firstkey(sdb);
#else
    key = firstkey();
#endif
	while (key.dptr) {
#if defined(NDBM)
		sinfo = dbm_fetch(sdb, key);
#else
		sinfo = fetch(key);
#endif
		bcopy(sinfo.dptr, &old, sizeof (struct stats));
		if (old.st_entries > 0) {
sprintf(buf, "%-24s %8d %6.2f  %6.2f  %5d\n",
			key.dptr,
			old.st_time,
			old.st_kills,
			old.st_maxkills,
			old.st_losses);
write(fid, buf, strlen(buf));
sprintf(buf, "%7d %5d %5d %7d %7d %6d %5d %7d %4d %5.3f\n",
			old.st_entries,
			old.st_conqs,
			old.st_coups,
			old.st_torps,
			old.st_phasers,
			old.st_armsbomb,
			old.st_armsship,
			old.st_planets,
			old.st_genocides,
			(old.st_losses ? old.st_kills / old.st_losses : (old.st_kills * 2.0)));
write(fid, buf, strlen(buf));
write(fid, "\n", 1);
		}
#if defined(NDBM)
		key = dbm_nextkey(sdb);
#else
		key = nextkey(key);
#endif
    }
#if defined(NDBM)
    (void) dbm_close(sdb);
#else
    dbmclose();
#endif
}
