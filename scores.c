static char sccsid[] = "@(#)scores.c	3.1";

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

#ifdef hpux
#include <fcntl.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include "defs.h"
#include <ndbm.h>


void
listscores(fid)
int	fid;
{
    DBM		*sdb;
    datum	key, sinfo;
    char	buf[BUFSIZE];
    struct stats old;

    sdb = dbm_open(SCOREFILE, O_RDONLY, 0700);
    if (sdb == (DBM *) NULL) {
	strcpy(buf, "listscores: Can't open SCOREFILE\n", strlen(buf));
	write(fid, buf, strlen(buf));
	return;
    }

  /*         0123456789012345678901234567890123456789012345678901234567890*/
strcpy(buf, "Name                     rtime     kills  maxkills  losses\n");
	write(fid, buf, strlen(buf));
strcpy(buf, "entries conqs coups   torps phasers  abomb aship planets gens ratio\n\n");
	write(fid, buf, strlen(buf));

    key = dbm_firstkey(sdb);
	while (key.dptr) {
		sinfo = dbm_fetch(sdb, key);
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
		key = dbm_nextkey(sdb);
    }
}
