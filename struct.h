/* static char sccsid[] = "@(#)struct.h	3.1"; */
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

#define	PFREE		0
#define	POUTFIT		1
#define	PALIVE		2
#define	PDEAD		3
#define	PSETUP		4
#define	PMAP		5

/* Return values from newwin(): */
#define	OK		1
#define	NODISPLAY	2
#define	NOFONTS		3

#define	EXPLODE		(PSETUP+1)	/* for s_status */
#define SFSHIELD	0x0001
#define SFREPAIR	0x0002
#define SFBOMB		0x0004
#define SFORBIT		0x0008
#define SFCLOAK		0x0010
#define SFWEP		0x0020
#define SFENG		0x0040
#define SFBEAMUP	0x0100
#define SFBEAMDOWN	0x0200
#define SFSELFDEST	0x0400
#define SFGREEN		0x0800
#define SFYELLOW	0x1000
#define SFRED		0x2000
#define SFSLOCK		0x4000		/* Locked on a ship */
#define SFPLLOCK	0x8000		/* Locked on a planet */
#define	SFWAR		0x200000	/* computer reprogramming for war */
#define	SFTOWING	0x800000	/* towing a ship */
#define	SFTOWED		0x1000000	/* being towed */
#define	SFDCLOAK	0x2000000	/* "damaged" - cloak is off */
#define	SFPLANETPROT	0x4000000	/* Go protect a planet */

#define	PFNOMAPMODE	0x0010		/* No map mode */
#define PFROBOT		0x0080		/* Robot player */
#define	PFPRACTICER	0x0100		/* Practice robot */
#define PFCOPILOT	0x10000		/* Allow copilots */
#define PFRHOSTILE	0x20000		/* Hostile robot */
#define PFRHARD		0x40000		/* hard robot */
#define	PFRSTICKY	0x80000		/* sticky robot */
#define	PFRVHARD	0x100000	/* very hard robot */
#define	PFSHOWSTATS	0x200000	/* show player statistics */
#define	PFRSHOWSTATS	0x400000	/* Resource set - show player statistics */
#define	PFSHOWSHIELDS	0x800000	/* show player shields */
#define	PFENTER		0x1000000	/* entry windows showing */
#define	PFNOTSHOWING	0x2000000	/* Players windows are not visible */

#define KQUIT		1		/* Player quit */
#define KTORP		2		/* killed by torp */
#define KPHASER		3		/* killed by phaser */
#define KPLANET		4		/* killed by planet */
#define KSHIP		5		/* killed by other ship */
#define KDAEMON		6		/* killed by dying daemon */
#define KWINNER		7		/* killed by a winner */
#define	KNEGENB		8		/* negative energy barrier */
#define	KNEWGAME	9		/* new game */
#define	KEVICT		10		/* game closed for repairs */
#define	KDOOMSDAY	11		/* planet eater got you */
#define	KGDOOMSDAY	12		/* you got the planet eater */
#define	KGOD		13		/* killed by the Lord */
#define	KLIGHTNING	14		/* killed by a lightning bolt from God */
#define	KVAPOR		15		/* vaporized - SIGPIPE on display */

#define	UMSGADDRLEN	10		/* user message address length */
#define	UMSGLEN		80		/* user message length */

struct universe {
	int	flags;
	char	*god_names[MAXPLAYER];
};
#define	WRAPAROUND	0x0001		/* allow ships to wrap around */

struct stats {
    int st_id;			/* identifier for this stats record */
    int st_time;		/* real time in game */
    double st_kills;		/* how many kills */
    int st_losses;		/* times killed */
    double st_maxkills;		/* times killed */
    int st_entries;		/* times in game */
    int st_conqs;		/* times galaxy taken over */
    int st_coups;		/* retaken home planet */
    int st_torps;		/* torps launched */
    int st_phasers;		/* phasers fired */
    int st_armsbomb;		/* armies bombed */
    int st_armsship;		/* ship board armies killed */
    int st_planets;		/* planets conquered */
    int st_genocides;		/* races genocided */
};

struct ship {
    int	s_no;
    int s_updates;		/* Number of updates ship has survived */
    int	s_delay;		/* Number of updates ship has to delay */
    int s_status;
    int	s_flags;
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

    short s_dcloak;
    short s_dcloak_on;
    short s_dcloak_off;

    int s_x;
    int s_y;
    unsigned char s_dir;	/* Real direction */
    unsigned char s_desdir;	/* desired direction */
    int s_subdir;		/* fraction direction change */
    short s_speed;		/* Real speed */
    short s_desspeed;		/* Desired speed */
    int s_subspeed;		/* Fractional speed */
    short s_team;		/* Team I'm on */
    short s_damage;		/* Current damage */
    int s_subdamage;		/* Fractional damage repair */
    short s_shield;		/* Current shield power */
    int s_subshield;		/* Fractional shield recharge */
    short s_ntorp;		/* Number of torps flying */
    int s_hostile;		/* Who my torps will hurt */
    int s_swar;			/* Who am I at sticky war with */
    short s_planet;		/* Planet orbiting or locked onto */
    short s_shipl;		/* Ship locked onto */
    short s_armies;
    short s_fuel;
    short s_explode;		/* Keeps track of final explosion */
    short s_etemp;
    short s_etime;
    short s_wtemp;
    short s_wtime;
    short s_timer;		/* General purpose timer */
    short s_rmode;		/* Robot attack mode */
    short s_whydead;		/* Tells you why you died */
    short s_whodead;		/* Tells you who killed you */
    int s_selfdest;		/* How long until I die. */
    int s_towing;		/* Ship number we are towing */
    int s_towed;		/* Ship we are being towed by */
    int	s_newhostile;
    int	s_reprogram;
    int s_oldalert;
    int	s_Ten;	/* Prepare for warp Ten+ */
    int	s_numpilots;		/* Number of pilots - normally one */
    int s_planetprot;		/* Number of planet to protect */
    struct stats s_stats;
};

typedef struct record {
	short	*data;
	short	last_value;
} RECORD;

/* This is a structure used for objects returned by mouse pointing */

#define PLANETTYPE 0x1
#define PLAYERTYPE 0x2

struct obtype {
    int o_type;
    int o_num;
    int o_dist;
};

struct player {
    int p_no;
    int p_status;		/* Player status */
    unsigned long p_flags;	/* Player flags */
    char p_name[16];
    char p_login[16];
    char p_monitor[16];		/* Monitor being played on */
    char p_mapchars[2];		/* Cache for map window image */
    char p_shipname[32];	/* Name of ship to search for in .Xdefaults */
    int	p_pick;			/* Teams I can pick from */

    int p_lastm;
    int p_mdisplayed;
    int p_lastcount;
    struct usermsg {		/* User message */
	char m_pending;
	char m_addr;
	char m_addrmsg[UMSGADDRLEN];
	char m_buf[UMSGLEN];
	int m_lcount;
    } p_umsg;
    int	p_warntimer;
    int	p_warncount;

    int p_mapmode;
    int p_namemode;
    int p_redrawall;
    int p_statmode;
    int p_infomapped;
    struct obtype	p_info_target;
    int p_ts_offset;
    RECORD	*p_rp;
    struct stats p_stats;	/* player statistics */
    int	p_copilot;		/* Which ship are we copiloting. */
    long	p_start_time;

    int		p_xwinsize;
    int		p_ywinsize;
    int		p_border;
    int		p_messagesize;	/* Height of message windows */

    struct ship *p_ship;	/* Personal ship statistics */

    /* X11 stuff: */
    Display	*display;
    int		focus;	/* Has focus or not... */
    int		screen;
    int		xcn;	/* XConnectionNumber() */
    int		mono;
    XFontStruct	*dfont, *bfont, *ifont, *xfont, *bigFont;
    Window	w, mapw, statwin, baseWin, messagew, infow, iconWin, tstatw,
		war, warf, warr, wark, waro, wargo, warno, warnw, helpWin,
		planetw, playerw, fwin, rwin, kwin, owin, qwin;
    GC		gc, bmgc, cleargc, dfgc, bfgc, ifgc, bFgc, xfgc, tbgc, dimgc;
    unsigned long borderColor, backColor, textColor, myColor,
		warningColor, shipCol[MAXTEAM+1], rColor, yColor, gColor, unColor;
    unsigned long moonColor, sunColor;
    Atom	wm_delete_window;
    Atom	wm_change_state;

    XRectangle	clearzone[(MAXTORP + 1) * MAXPLAYER + MAXPLANETS];
    int		clearcount;
    int		clearline[4][MAXPLAYER];
    int		clearlcount;
    XRectangle	mclearzone[MAXPLAYER];
    int		mclearcount;
    XRectangle	mpzone[MAXPLANETS * 2];
    int		mpzset[MAXPLANETS];
    int		textWidth;
    int		statX;
    int		statY;
    int		uspec;
    GC		sgc;
    Pixmap	foreTile, backTile, rTile, yTile, gTile, stippleTile;
    Pixmap	ibm;	/* Icon bitmap */
    Pixmap	tbm;
    Window	clockw;
    int		once;
    int		oldtime;
    long	startTime;
    int		mustexit;
};

/* Torpedo states */

#define TFREE 0
#define TMOVE 1
#define TEXPLODE 2
#define TDET 3
#define TOFF 4
#define TSTRAIGHT 5		/* Non-wobbling torp */

struct torp {
    int t_no;
    int t_status;		/* State information */
    int t_owner;
    int t_x;
    int t_y;
    unsigned char t_dir;	/* direction */
    int t_damage;		/* damage for direct hit */
    int t_speed;		/* Moving speed */
    int t_fuse;			/* Life left in current state */
    int t_war;			/* enemies */
    int t_team;			/* launching team */
};

#define PHFREE 0x00
#define PHHIT  0x01
#define PHMISS 0x02

struct phaser {
    int ph_status;		/* What it's up to */
    unsigned char ph_dir;	/* direction */
    int ph_target;		/* Who's being hit (for drawing) */
    int ph_fuse;		/* Life left for drawing */
    int ph_damage;		/* Damage inflicted on victim */
};

/* the lower bits represent the original owning team */
#define PLHOME		0x080		/* home planet for a given team */
#define PLCOUP		0x100		/* Coup has occured */
#define	PLINVIS		0x200		/* planet is invisible */

/* planet types: */
#define	CLASSM		1
#define	DEAD		2
#define	SUN		3
#define	MOON		4
#define	GHOST		5

struct planet {
    int pl_no;
    int pl_flags;		/* State information */
    int pl_owner;
    int pl_x;
    int pl_y;
    char pl_name[16];
    int pl_namelen;		/* Cuts back on strlen's */
    int pl_armies;
    int pl_info;		/* Teams which have info on planets */
    int pl_deadtime;		/* Time before planet will support life */
    int pl_couptime;		/* Time before coup may take place */
    int pl_drawtime;		/* Time when map should be redrawn */

    char pl_tname[10];		/* Type name */
    int pl_tnamelen;
    int pl_primary;		/* planet this planet orbits */
    int pl_orbrad;		/* radius of the orbit */
    int pl_orbvel;		/* velocity of the orbit */
    int pl_type;		/* planet type */
    int pl_orbang;		/* current angle of the orbit */
    int pl_suborbang;		/* amount of a partial angle we've orbitted */
};

#define MVALID 0x01
#define MINDIV 0x02
#define MTEAM  0x04
#define MALL   0x08

struct message {
    int m_no;
    int m_flags;
    int m_time;
    int m_recpt;
    char m_from[3];
    char m_to[3];
    char m_data[80];
};

/* message control structure */

struct mctl {
    int mc_current;
};
