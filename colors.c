static char sccsid[] = "@(#)colors.c	3.1";

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

#include <stdio.h>
#include <X11/Xlib.h>
#include <string.h>
#include "defs.h"
#include "data.h"

extern int	debug;

typedef struct assoc {
	char		*str;
	int		bWDef;
	char		*colorDef;
} ASSOC;

#define BLACKPIXEL	0
#define WHITEPIXEL	1

ASSOC	assoc[] = {
	{ "border",	WHITEPIXEL,	"blue"		},
	{ "background",	BLACKPIXEL,	"black"		},
	{ "text",	WHITEPIXEL,	"white"		},
	{ "nobody",	WHITEPIXEL,	"blue"		},
	{ "fed",	WHITEPIXEL,	"yellow",	},
	{ "rom",	WHITEPIXEL,	"red",		},
	{ "kli",	WHITEPIXEL,	"green",	},
	{ "ori",	WHITEPIXEL,	"#0ff",		},
	{ "ind",	WHITEPIXEL,	"brown"		},
	{ "noteam",	BLACKPIXEL,	"black"		},
	{ "god",	WHITEPIXEL,	"white"		},
	{ "warning",	WHITEPIXEL,	"red"		},
	{ "unknown",	WHITEPIXEL,	"light grey"	},
	{ "ralert",	WHITEPIXEL,	"red"		},
	{ "yalert",	WHITEPIXEL,	"yellow"	},
	{ "galert",	WHITEPIXEL,	"green"		},
	{ "me",		WHITEPIXEL,	"white"		},
	{ "moon",	WHITEPIXEL,	"dim grey"	},
	{ "sun",	WHITEPIXEL,	"gold"		}
};

getColorDefs(p, prog)
register struct player	*p;
char			*prog;
{
	int	i, invert = 0;
	char	*color;
	Colormap	default_colormap;
	unsigned long	*pp;

	XColor		def;
	unsigned long	white_pix, black_pix;

	if (p->mono) {	/* b & w */
		white_pix = XWhitePixel(p->display, p->screen);
		black_pix = XBlackPixel(p->display, p->screen);
		invert = booleanDefault(p, prog, "reverseVideo");
		for (i = 0; i < sizeof (assoc) / sizeof (*assoc); i++) {
			switch (i) {
				case 0:	pp = &p->borderColor;	break;
				case 1:	pp = &p->backColor;	break;
				case 2:	pp = &p->textColor;	break;
				case 3:	pp = &p->shipCol[0];	break;
				case 4:	pp = &p->shipCol[1];	break;
				case 5:	pp = &p->shipCol[2];	break;
				case 6:	pp = &p->shipCol[3];	break;
				case 7:	pp = &p->shipCol[4];	break;
				case 8:	pp = &p->shipCol[5];	break;
				case 9:	pp = &p->shipCol[6];	break;
				case 10:pp = &p->shipCol[7];	break;
				case 11:pp = &p->warningColor;	break;
				case 12:pp = &p->unColor;	break;
				case 13:pp = &p->rColor;	break;
				case 14:pp = &p->yColor;	break;
				case 15:pp = &p->gColor;	break;
				case 16:pp = &p->myColor;	break;
				case 17:pp = &p->moonColor;	break;
				case 18:pp = &p->sunColor;	break;
			}
			if (!invert)
				*pp = (assoc[i].bWDef == BLACKPIXEL)
					? black_pix : white_pix;
			else
				*pp = (assoc[i].bWDef == BLACKPIXEL)
					? white_pix : black_pix;
		}
	} else {
		default_colormap = XDefaultColormap(p->display, p->screen);
		for (i = 0; i < sizeof (assoc) / sizeof (*assoc); i++) {
			switch (i) {
				case 0:	pp = &p->borderColor;	break;
				case 1:	pp = &p->backColor;	break;
				case 2:	pp = &p->textColor;	break;
				case 3:	pp = &p->shipCol[0];	break;
				case 4:	pp = &p->shipCol[1];	break;
				case 5:	pp = &p->shipCol[2];	break;
				case 6:	pp = &p->shipCol[3];	break;
				case 7:	pp = &p->shipCol[4];	break;
				case 8:	pp = &p->shipCol[5];	break;
				case 9:	pp = &p->shipCol[6];	break;
				case 10:pp = &p->shipCol[7];	break;
				case 11:pp = &p->warningColor;	break;
				case 12:pp = &p->unColor;	break;
				case 13:pp = &p->rColor;	break;
				case 14:pp = &p->yColor;	break;
				case 15:pp = &p->gColor;	break;
				case 16:pp = &p->myColor;	break;
				case 17:pp = &p->moonColor;	break;
				case 18:pp = &p->sunColor;	break;
			}
			if ((color = XGetDefault(p->display, PROGRAM_NAME, assoc[i].str))
			    == NULL)
				color = assoc[i].colorDef;
			def.pixel = 0;
			invert = XParseColor(p->display, default_colormap, color, &def);
			invert = XAllocColor(p->display, default_colormap, &def);
			*pp = def.pixel;
		}
	}
}

booleanDefault(p, prog, def)
register struct player	*p;
char			*prog, *def;
{
	char	*str;

	/*
	 * jas (Jeff Schmidt)  Allow for either 'on' or 'true' to indicate the
	 * boolean TRUE state.
	 */

	if (debug)
		fprintf(stderr, "bD: pno %d", p->p_no);

	if ((str = XGetDefault(p->display, prog, def)) != NULL) {
	    if (debug)
		fprintf(stderr, " def(%s) str(%s)\n", def, str);
	    if ((strcmp(str, "on") == 0) || (strcmp(str, "true") == 0))
		return (1);
	    return (0);
	}

	if (debug)
		fprintf(stderr, " def(%s) str(%s)\n", def, "NULL");

	return (0);
}

#define iswhite(c)	((c) == ' ' || c == '\t' || (c) == ',')

arrayDefault(p, prog, def, sizeP, sp)
register struct player	*p;
char			*prog, *def;
int			*sizeP;
char			*sp;
{
	int	max;
	char	*str;
	int	rc;

	str = XGetDefault(p->display, PROGRAM_NAME, def);
	if (str == NULL)
		return (-1);
	max = *sizeP;
	*sizeP = 0;

	for (;;) {
		while (iswhite(*str))
			str++;
		if (*str == '\0')
			break;
		if (++(*sizeP) > max)
			return (-1);
		if (sscanf(str, "0x%x", &rc) != 1)
			return (-1);
		sp[*sizeP] = rc;
		while (!iswhite(*str) && *str != '\0')
			str++;
	}
	return (0);
}
