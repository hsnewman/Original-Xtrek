static char sccsid[] = "@(#)DBG.c	3.1";
#include <X11/Xlib.h>
#include "defs.h"
static int DEBUG = 1;

DBG(p, fmt, a1)
register struct player	*p;
register char *fmt;
register unsigned a1;
{
	if (p && p->display)
		XSync(p->display, p->screen);
	if (DEBUG)
		printf("%s %d\n", fmt, a1);
}
