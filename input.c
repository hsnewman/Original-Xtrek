static char sccsid[] = "@(#)input.c	3.1";
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
#include <X11/Xutil.h>
#include <stdio.h>
#include <math.h>
#if !defined(cray)
#include <sys/types.h>
#endif
#include <sys/socket.h>

#ifdef SUN40
#include <sys/filio.h>
#else SUN40
#if !defined(cray)
# define	FD_SET(n, s)	(((s)->fds_bits[0]) |= (1 << n))
# define	FD_CLR(n, s)	(((s)->fds_bits[0]) &= ~(1 << n))
# define	FD_ZERO(s)	bzero((char *)(s), sizeof (*(s)))
# define	FD_ISSET(n, s)	(((s)->fds_bits[0]) & (1 << n))
#endif
#include <sys/ioctl.h>
#endif

#ifdef hpux
#include <time.h>
#else hpux
#include <sys/time.h>
#endif hpux

#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include "defs.h"
#include "data.h"

static int		doTheRedrawDude, skipUpdates = 1;
static int		peerdied;
static struct player	*xpendwho;
struct player	*vapor[MAXPLAYER];
jmp_buf		xpendenv;
int		jumpable = 0;
int		vapor_cnt;

extern Window	openStats();
extern char	*newwin();
extern char	*winmapped();
extern int	debug;

initinput(p)
register struct player	*p;
{
    XSelectInput(p->display, p->iconWin, ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->w,
	KeyPressMask|ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask|FocusChangeMask|EnterWindowMask|LeaveWindowMask);
    XSelectInput(p->display, p->mapw,
	KeyPressMask|ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask|FocusChangeMask|EnterWindowMask|LeaveWindowMask);
    XSelectInput(p->display, p->messagew,
	KeyPressMask|ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask|FocusChangeMask|EnterWindowMask|LeaveWindowMask);
    XSelectInput(p->display, p->tstatw, ExposureMask|SubstructureNotifyMask);

    XSelectInput(p->display, p->war, ExposureMask|SubstructureNotifyMask);

    XSelectInput(p->display, p->warf, ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->warr, ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->wark, ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->waro, ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->wargo, ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->warno, ButtonPressMask|ButtonReleaseMask|ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->helpWin, ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->planetw, ExposureMask|SubstructureNotifyMask);
    XSelectInput(p->display, p->playerw, ExposureMask|SubstructureNotifyMask);
}

int	loopcounter;
int	maxloopcounter;

setRedrawFlag()
{
	if (skipUpdates)
		doTheRedrawDude = 1;
	else
		doTheRedrawDude++;
	if (maxloopcounter < loopcounter)
		maxloopcounter = loopcounter;
	loopcounter = 0;
}

void
deadpeer()
{
	if (xpendwho) {
		vapor[vapor_cnt++] = xpendwho;
		xpendwho = (struct player *) NULL;
	} else
		peerdied = 1;

	if (debug)
		fprintf(stderr, "Got SIGPIPE, vapor_cnt=%d, jump=%d\n", vapor_cnt, jumpable);

	signal(SIGPIPE, deadpeer);

	if (jumpable)
		longjmp(xpendenv, 1);
}

int	playerchange;
int	nplayers;
int	nships;
int	nrobots;

input()
{
    struct player	*p;
    int		pno;
    XEvent	data;
    XEvent	*tmp;
    XDestroyWindowEvent		*xdwe;
    char	buf[128];
    char	sbuf[128];
    char	hostname[80];
    char	new_display[50], new_login[50], new_shipname[50];
    int		nchar;
    fd_set	ofdset, fdset;
    struct itimerval	udt;
    int			didevent;
    long		elapsed;
    int			noplayer_updates;
    int			query_wait;
    struct sockaddr	addr;
    int			addrlen;
    int			ns;
    int			on = 1;
    int			si;
    int			copilot;
    int			lastmaxloopcounter;
    int			running_away, eblocks_rcvd;
    XEvent		clmsg;
    int			val;
    long		xxxtime;
    extern char		*sys_errlist[];

/*
 * jas (Jeff Schmidt)  Determine server host name.
 */

    gethostname(hostname, sizeof(hostname));

    signal(SIGALRM, setRedrawFlag);
#if !defined(cray)
    udt.it_interval.tv_sec = 0L;
    udt.it_interval.tv_usec = UPDATE;
    udt.it_value.tv_sec = 0L;
    udt.it_value.tv_usec = UPDATE;
    setitimer(ITIMER_REAL, &udt, (struct itimerval *) NULL);
#else
    ms_timer(UPDATE / 1000);
#endif

    tmp = &data;

    FD_ZERO(&ofdset);
    FD_ZERO(&fdset);
    playerchange = 1;
    nplayers = 0;
    nships = 0;
    nrobots = 0;
    noplayer_updates = 0;
    query_wait = 0;
    didevent = 1;
    ns = -1;
    vapor_cnt = 0;
    signal(SIGPIPE, deadpeer);
    loopcounter = 0;
    maxloopcounter = 0;
    lastmaxloopcounter = 0;
    running_away = 0;
    eblocks_rcvd = 0;
    while (1) {
	loopcounter++;
	if (maxloopcounter > lastmaxloopcounter) {
		lastmaxloopcounter = maxloopcounter;
		fprintf(stderr, "MAXLOOPCOUNTER=%d, RAN=%d, BLOCKS=%d, de=%d, pc=%d, np=%d\n", maxloopcounter, running_away, eblocks_rcvd, didevent, playerchange, nplayers);
	}
#ifndef NO_INETD
	/* NOTE: noplayer_updates only increments when nplayers is zero, */
	/*	Also, query_wait only decrements when noplayer_updates is */
	/*	over the limit. */
	if (ns < 0 && !nplayers && (noplayer_updates++ > (30 * UPS)) && (query_wait-- <= 0)) {
		save_planets();
#if !defined(NEWINET)
		drain_udp_socket();
#endif
		time(&xxxtime);
		fprintf(stderr, "in.xtrekd: exiting at %s\n", ctime(&xxxtime));
		exit(0);
	}
#endif /* NO_INETD */
	/* Handle the people who vaporized. */
	while (vapor_cnt) {
		p = vapor[--vapor_cnt];
		sprintf(buf, "%s (%c%x) vaporized", p->p_name,
			teamlet[p->p_ship->s_team], p->p_ship->s_no);
		pmessage(buf, 0, MALL, "GOD->ALL");
		if (debug)
			fprintf(stderr, "%s\n", buf);
		if (!p->p_copilot) {
			p->p_ship->s_whydead = KVAPOR;
			p->p_ship->s_whodead = p->p_ship->s_no;
			death(p);
			drop_copilots(p);
			p->p_ship->s_numpilots = 0;
		} else {
			calcstats(p);
			savestats(p);
			p->p_ship->s_numpilots--;
		}
		FD_CLR(p->xcn, &ofdset);
		close(p->xcn);
		p->xcn = 0;
		p->p_status = PFREE;
		p->p_warncount = 0;
	}
	if (!didevent) {
		fdset = ofdset;
		pno = select(32, &fdset, (fd_set *) NULL, (fd_set *) NULL, (struct timeval *) NULL);
		if (pno < 0) {
			FD_CLR(xtrek_socket, &fdset);
			if (ns > 0)
				FD_CLR(ns, &fdset);
		}
	}
	if (FD_ISSET(xtrek_socket, &fdset) || (ns > 0 && FD_ISSET(ns, &fdset))) {
		addrlen = sizeof (addr);
		errno = 0;
		if (ns == -1) {
			ns = accept(xtrek_socket, &addr, &addrlen);
			if (ns > 0) {
				si = 0;
				peerdied = 0;
				ioctl(ns, FIONBIO, &on);
				FD_SET(ns, &ofdset);
			}
		}
		errno = 0;
		if (ns > 0 && FD_ISSET(ns, &fdset)) {
			pno = read(ns, &sbuf[si], 1);
			if (pno > 0)
				si += pno;
			if (pno == 0)
				peerdied = 1;
			if (pno < 0) {
				if (errno == EWOULDBLOCK) {
					eblocks_rcvd++;
					playerchange = 1;
				} else {
					fprintf(stderr, "input, read: %s\n", sys_errlist[errno]);
					close(ns);
					ns = -1;
					peerdied = 1;
					running_away++;
				}
			} else
				eblocks_rcvd = 0;

			if (pno > 0 && si >= 2 && sbuf[si-2] == '\015' && sbuf[si-1] == '\012') {
				sbuf[si-2] = '\0';
				copilot = MAXPLAYER;
				if (strcmp(sbuf, "Query?") == 0) {
					sprintf(sbuf, "F: %d, R: %d, K: %d, O:%d\n",
					     tcount[FED], tcount[ROM], tcount[KLI], tcount[ORI]);
					write(ns, sbuf, strlen(sbuf));
					/* Don't stop daemon for at least 5 minutes. */
					query_wait = 300 * UPS;
					close(ns);
					ns = -1;
				} else if (strcmp(sbuf, "RScore?") == 0) {
					scorelist((struct player *) NULL, ns);
					/* Don't stop daemon for at least 5 minutes. */
					query_wait = 300 * UPS;
					close(ns);
					ns = -1;
				} else if (strcmp(sbuf, "Players?") == 0) {
					playerlist((struct player *) NULL, ns);
					/* Don't stop daemon for at least 5 minutes. */
					query_wait = 300 * UPS;
					close(ns);
					ns = -1;
				} else if (strcmp(sbuf, "LScore?") == 0) {
					liststats(ns);
					/* Don't stop daemon for at least 5 minutes. */
					query_wait = 300 * UPS;
					close(ns);
					ns = -1;
				} else if (sscanf(sbuf, "Display: %s Login: %s Shipname: %s Copilot: %x", new_display, new_login, new_shipname, &copilot) != 4) {
					write(ns, "Bad format\n", 11);
					close(ns);
					ns = -1;
				} else {
					/* Trying to be copilot? */
					if (copilot < MAXPLAYER) {
						if (players[copilot].p_status != PALIVE) {
							sprintf(sbuf, "Ship %x is not alive.\n", copilot);
							write(ns, sbuf, strlen(sbuf));
							close(ns);
							ns = -1;
							goto noconnect;
						} else if (!(players[copilot].p_flags & PFCOPILOT)) {
							sprintf(sbuf, "Ship %x is not allowing copilots.\n", copilot);
							write(ns, sbuf, strlen(sbuf));
							close(ns);
							ns = -1;
							goto noconnect;
						}
					}
					sprintf(sbuf, "Connecting %s -> %s\n",
					  hostname, new_display);
					write(ns, sbuf, strlen(sbuf));
					pno = findslot();
					if (pno == MAXPLAYER) {
						write(ns, "No more room in game\n", 21);
					} else {
						strcpy(players[pno].p_monitor, new_display);
						strcpy(players[pno].p_login, new_login);
						strcpy(players[pno].p_shipname, new_shipname);
						if (copilot < MAXPLAYER) {
							char xp[32];

							players[pno].p_ship = &ships[copilot];
							ships[copilot].s_numpilots++;
							players[pno].p_copilot = 1;
							sprintf(sbuf, "%s entered as a copilot for %c%1x",
								players[pno].p_login,
								teamlet[players[pno].p_ship->s_team],
								copilot);
							sprintf(xp, "GOD->%s", teamshort[players[pno].p_ship->s_team]);
							pmessage(sbuf, players[pno].p_ship->s_team, MTEAM, xp);
						} else {
							players[pno].p_ship = &ships[pno];
							ships[pno].s_team = 0;
							ships[pno].s_numpilots = 1;
							players[pno].p_copilot = 0;
						}
						players[pno].p_status = PSETUP;
						sprintf(sbuf, "%s, you are player number %d, %siloting ship number %d\n",
							new_login, pno,
							players[pno].p_copilot ? "Cop" : "P",
							players[pno].p_ship->s_no);
						write(ns, sbuf, strlen(sbuf));
					}
				}
noconnect:
				playerchange = 1;
			}
		}
	}
	if (peerdied) {
		peerdied = 0;
		close(ns);	/* Trying to get that runaway bug... */
		ns = -1;	/* Trying to get that runaway bug... */
		playerchange = 1;
	}
	if (playerchange) {
		FD_ZERO(&ofdset);
		FD_SET(xtrek_socket, &ofdset);
		if (ns > 0)
			FD_SET(ns, &ofdset);
		nplayers = 0;
		nships = 0;
		nrobots = 0;
		for (pno = 0, p = &players[pno]; pno < MAXPLAYER; pno++, p++) {
			if (p->p_status != PFREE) {
				nplayers++;
				if (p->p_ship->s_no == pno)
				    nships++;
				if ((p->p_flags & PFROBOT) != 0)
					nrobots++;
				else if (p->p_status != PSETUP)
					FD_SET(p->xcn, &ofdset);
			}
		}
		if (nplayers)
			noplayer_updates = 0;
		playerchange = 0;
	}

	while (doTheRedrawDude-- > 0) {
		/* NOTE: Don't update system when we are only hanging around */
		/*	for the sake of queries. */
		if (nplayers)
			intrupt();
	}

	didevent = 0;
	for (pno = 0, p = &players[0]; pno < MAXPLAYER; pno++, p++) {
		if (p->p_status == PFREE)
			continue;

		if (p->p_status == PSETUP) {
			char	*rval;

			playerchange = 1;
			rval = newwin(p);
			if (rval == (char *) NULL) {		/* Create new windows */
				p->p_pick = ((1<<FED)|(1<<ROM)|(1<<KLI)|(1<<ORI));	/* Team's they can pick from */
				FD_SET(p->xcn, &ofdset);
				close(ns);	/* Still trying to get the bug */
				ns = -1;	/* Still trying to get the bug */
			} else {
				fprintf(stderr, "Player %d (%s) -> %s\n", p->p_no, p->p_name ? p->p_name : "", rval);
				write(ns, rval, strlen(rval));
				write(ns, "\n", 1);
				if (p->display)
					XCloseDisplay(p->display);
				p->display = (Display *) NULL;
				p->p_ship->s_numpilots--;
				p->p_status = PFREE;
				p->p_warncount = 0;
				close(ns);
				ns = -1;
				continue;
			}
#ifdef notdef
			close(ns);
			ns = -1;
#endif
			if (p->p_status == PMAP)
				mapBase(p);		/* Map the baseWin... */
		}

		if (p->p_status == POUTFIT && p->p_copilot) {
			playerchange = 1;
			p->p_status = PALIVE;
			enter(p->p_ship->s_team, XDisplayString(p->display), p->p_no);
		}

		if (p->p_status == POUTFIT && (!p->p_copilot)) {
			playerchange = 1;
			if (!(p->p_flags & PFENTER)) {
				p->p_flags |= PFENTER;
				entrywindow(p);	/* Show them the entry window */
			} else {
				elapsed = time(0) - p->startTime;
				if (elapsed > AUTOQUIT) {
					playerchange = 1;
					XCloseDisplay(p->display);
					p->display = (Display *) NULL;
					p->p_status = PFREE;
					drop_copilots(p);
					p->p_ship->s_numpilots = 0;
					p->p_warncount = 0;
					p->p_flags &= ~PFENTER;
					continue;
				} else {
					showTimeLeft(p, elapsed, AUTOQUIT, 0);
					redrawFed(p, p->fwin, 0);
					redrawRom(p, p->rwin, 0);
					redrawKli(p, p->kwin, 0);
					redrawOri(p, p->owin, 0);
				}
			}
		}

		if (!(FD_ISSET(p->xcn, &fdset))) {
			continue;
		}

		if (p->p_flags & PFROBOT)
			continue;

		/* Prepare for vaporizing people... */
		xpendwho = p;
		jumpable = 1;
		if (setjmp(xpendenv)) {
			if (!xpendwho) {
				/* Yup, they just vaporized. */
				continue;
			}
		}
		if (p->display == (Display *) NULL || !XPending(p->display)) {
			if (!xpendwho)
				continue;
			if (!p->display)
				fprintf(stderr, "Null display in input.\n");
			continue;
		}

		xpendwho = p;
		jumpable = 1;
		if (setjmp(xpendenv)) {
			if (!xpendwho) {
				continue;
			}
		}

		XNextEvent(p->display, (XEvent *)&data);	/* grab the event */
		jumpable = 0;

		didevent = 1;

		if (p->p_ship->s_updates > p->p_ship->s_delay) {
		    p->p_ship->s_flags &= ~(SFWAR);
		}

		switch ((int) data.type) {
			/* Types to ignore... */
		    case KeyRelease:
		    case ButtonRelease:
		    case CreateNotify:
		    case ReparentNotify:
		    case FocusOut:
			break;

		    case FocusIn:
			/* Something is showing... */
			p->p_flags &= ~PFNOTSHOWING;
			break;
		    case VisibilityNotify:
			if (data.xvisibility.window == p->baseWin) {
				if (data.xvisibility.state == VisibilityFullyObscured) {
					p->p_flags |= PFNOTSHOWING;
				} else {
					p->p_flags &= ~PFNOTSHOWING;
				}
			}
			break;

		    case UnmapNotify:
			if (debug && data.xmap.window == p->iconWin) {
				fprintf(stderr, "Got UnmapNotify event for iconWin\n");
			}
			if (data.xmap.window == p->baseWin) {
				if (debug)
					fprintf(stderr, "Got UnmapNotify event for baseWin\n");
				clmsg.xclient.type = ClientMessage;
				clmsg.xclient.send_event = True;
				clmsg.xclient.display = p->display;
				clmsg.xclient.window = p->baseWin;
				clmsg.xclient.message_type = p->wm_change_state;
				clmsg.xclient.format = 32;
				clmsg.xclient.data.l[0] =  IconicState;
				val = XSendEvent(p->display, RootWindow(p->display, p->screen), False, (SubstructureRedirectMask|SubstructureNotifyMask), &clmsg);
				if (debug)
					fprintf(stderr, "XSendEvent(ClientMessage) = %d\n", val);
			}
			break;

		    case EnterNotify:
			p->focus = 0;
			if (data.xcrossing.window == p->w) {
				p->focus = 1;
				XSetInputFocus(p->display, p->w, RevertToParent, data.xcrossing.time);
			} else if (data.xcrossing.window == p->mapw) {
				p->focus = 1;
				XSetInputFocus(p->display, p->mapw, RevertToParent, data.xcrossing.time);
			} else if (data.xcrossing.window == p->messagew) {
				p->focus = 1;
				XSetInputFocus(p->display, p->messagew, RevertToParent, data.xcrossing.time);
			}
			break;

		    case LeaveNotify:
			p->focus = 0;
			break;

		    case ClientMessage:
			if ((Atom) data.xclient.data.l[0] == p->wm_delete_window) {
				if (data.xclient.window == p->baseWin) {
					sprintf(buf, "%s (%c%x) vaporized", p->p_name,
						teamlet[p->p_ship->s_team], p->p_ship->s_no);
					pmessage(buf, 0, MALL, "GOD->ALL");
					if (!p->p_copilot) {
						p->p_ship->s_whydead = KVAPOR;
						p->p_ship->s_whodead = p->p_ship->s_no;
						death(p);
						drop_copilots(p);
						p->p_ship->s_numpilots = 0;
					} else {
						calcstats(p);
						savestats(p);
						p->p_ship->s_numpilots--;
					}
					playerchange = 1;
					XCloseDisplay(p->display);
					p->display = (Display *) NULL;
					p->p_status = PFREE;
					p->p_warncount = 0;
					p->p_flags &= ~PFENTER;
				} else if (data.xclient.window == p->helpWin) {
					XUnmapWindow(p->display, p->helpWin);
				} else if (data.xclient.window == p->statwin) {
					closeStats(p, p->statwin);
				}
			}
			break;

		    case DestroyNotify:
			xdwe = &data.xdestroywindow;
			if (debug)
				fprintf(stderr, "Got DestroyNotify for Player %d\n", p->p_no);
			if (xdwe->window == p->baseWin) {
				if (debug)
					fprintf(stderr, "Got DestroyNotify for Player %d on the baseWin\n", p->p_no);
			}
			if (xdwe->window == p->helpWin) {
				if (debug)
					fprintf(stderr, "Got DestroyNotify for Player %d on the helpWin\n", p->p_no);
			}
			break;

		    case KeyPress:
			if (!p->focus)
				break;
			if (p->p_status == POUTFIT && (!p->p_copilot)) {
				int	team;

				team = -1;
				if (data.xkey.window == p->fwin)
					team = FED;
				else if (data.xkey.window == p->rwin)
					team = ROM;
				else if (data.xkey.window == p->kwin)
					team = KLI;
				else if (data.xkey.window == p->owin)
					team = ORI;
				else if (data.xkey.window == p->qwin) {
					playerchange = 1;
					XCloseDisplay(p->display);
					p->display = (Display *) NULL;
					p->p_status = PFREE;
					drop_copilots(p);
					p->p_ship->s_numpilots = 0;
					p->p_warncount = 0;
					p->p_flags &= ~PFENTER;
				}
				if (team >= 0) {
					del_entrywindow(p);
					XClearWindow(p->display, p->w);
					p->p_flags &= ~PFENTER;
					enter(team, XDisplayString(p->display), pno);
					start_copilots(p);
					playerchange = 1;
				}
				continue;
			}
			if (inputIgnored(p))
			    continue;
			if (p->p_ship->s_flags & SFSELFDEST) {
			    p->p_ship->s_flags &= ~SFSELFDEST;
			    warning(p, "Self Destruct has been canceled");
			}
			nchar = XLookupString(&tmp->xkey, buf, sizeof buf,
				(KeySym *)NULL, (XComposeStatus *)NULL);
			if (nchar > 0) {
			    if (data.xkey.window == p->messagew)
				smessage(p, *buf);
			    else
				keyaction(p, *buf, &tmp->xkey);
			}
			break;

		    case ButtonPress:
			if (p->p_status == POUTFIT && (!p->p_copilot)) {
				int	team;

				team = -1;
				if (data.xkey.window == p->fwin)
					team = FED;
				else if (data.xkey.window == p->rwin)
					team = ROM;
				else if (data.xkey.window == p->kwin)
					team = KLI;
				else if (data.xkey.window == p->owin)
					team = ORI;
				else if (data.xkey.window == p->qwin) {
					playerchange = 1;
					XCloseDisplay(p->display);
					p->display = (Display *) NULL;
					p->p_status = PFREE;
					drop_copilots(p);
					p->p_ship->s_numpilots = 0;
					p->p_warncount = 0;
					p->p_flags &= ~PFENTER;
				}
				if (team >= 0) {
					del_entrywindow(p);
					XClearWindow(p->display, p->w);
					p->p_flags &= ~PFENTER;
					enter(team, XDisplayString(p->display), pno);
					start_copilots(p);
					playerchange = 1;
				}
				continue;
			}
			if (inputIgnored(p))
				continue;
			if (p->p_ship->s_flags & SFSELFDEST) {
			    p->p_ship->s_flags &= ~SFSELFDEST;
			    warning(p, "Self Destruct has been canceled");
			}
			if (data.xbutton.window == p->warf)
			    waraction(p, &tmp->xbutton);
			else if (data.xbutton.window == p->warr)
			    waraction(p, &tmp->xbutton);
			else if (data.xbutton.window == p->wark)
			    waraction(p, &tmp->xbutton);
			else if (data.xbutton.window == p->waro)
			    waraction(p, &tmp->xbutton);
			else if (data.xbutton.window == p->wargo)
			    waraction(p, &tmp->xbutton);
			else if (data.xbutton.window == p->warno)
			    waraction(p, &tmp->xbutton);
			else
			    buttonaction(p, &tmp->xbutton);
			break;


		    case Expose:
			if (p->p_status == POUTFIT && (!p->p_copilot)) {
				if (data.xexpose.window == p->fwin)
					redrawFed(p, p->fwin, 1);
				else if (data.xexpose.window == p->rwin)
					redrawRom(p, p->rwin, 1);
				else if (data.xexpose.window == p->kwin)
					redrawKli(p, p->kwin, 1);
				else if (data.xexpose.window == p->owin)
					redrawOri(p, p->owin, 1);
				else if (data.xexpose.window == p->w)
					showMotd(p);
				else if (data.xexpose.window == p->qwin) {
					redrawQuit(p, p->qwin);
					elapsed = time(0) - p->startTime;
					showTimeLeft(p, elapsed, AUTOQUIT, 1);
				}
			}
			if (data.xexpose.window == p->statwin && (p->p_flags & PFSHOWSTATS))
				redrawStats(p, p->statwin);
			else if (data.xexpose.window == p->tstatw)
				redrawTstats(p);
			else if (data.xexpose.window == p->mapw)
			    p->p_redrawall = 1;
			else if (data.xexpose.window == p->iconWin)
			    drawIcon(p);
			else if (data.xexpose.window == p->helpWin)
			    fillhelp(p);
			else if (data.xexpose.window == p->playerw)
			    playerlist(p, 0);
			else if (data.xexpose.window == p->planetw)
			    planetlist(p);
			else if (data.xexpose.window == p->infow)
			    drawinfo(p);
			else if (data.xexpose.window == p->war)
			    warrefresh(p);
			else if (data.xexpose.window == p->warf)
			    warfed(p);
			else if (data.xexpose.window == p->warr)
			    warrom(p);
			else if (data.xexpose.window == p->wark)
			    warkli(p);
			else if (data.xexpose.window == p->waro)
			    warori(p);
			else if (data.xexpose.window == p->wargo)
			    wargo(p);
			else if (data.xexpose.window == p->warno)
			    warno(p);
			break;

		    case MapNotify:
			if (p->p_status == PMAP && data.xmap.window == p->baseWin) {
				char *rval;

				rval = winmapped(p);
				if (rval != (char *) NULL) {		/* Create new windows */
					fprintf(stderr, "Player %d (%s) -> %s\n", p->p_no, p->p_name ? p->p_name : "", rval);
					write(ns, rval, strlen(rval));
					write(ns, "\n", 1);
/* Don't do this...it is done in getFonts. XCloseDisplay(p->display); */
					p->display = (Display *) NULL;
					p->p_ship->s_numpilots--;
					p->p_status = PFREE;
					p->p_warncount = 0;
					playerchange = 1;
				}
				close(ns);
				ns = -1;
			}
			if (debug && data.xmap.window == p->iconWin) {
				fprintf(stderr, "Got MapNotify event for iconWin\n");
			}
			if (debug && data.xmap.window == p->baseWin) {
				fprintf(stderr, "Got MapNotify event for baseWin\n");
			}
			break;

		    case ConfigureNotify:
			if (p->baseWin == data.xconfigure.window)
				resizewins(p, &data);
			break;

		    default:
			if (debug) {
				fprintf(stderr, "Got event %d\n", data.type);
			}
			break;
		} /* switch */
	}
    } /* (infinite) loop */
}

drop_copilots(p)
register struct player	*p;
{
	register struct player	*co;
	register int		i;

	for (i = 0, co = &players[0]; i < MAXPLAYER; i++, co++) {
		if (co->p_status == PFREE)
			continue;
		if (co->p_ship == p->p_ship && co->p_copilot) {
			playerchange = 1;
			XCloseDisplay(co->display);
			co->display = (Display *) NULL;
			co->p_status = PFREE;
			co->p_warncount = 0;
			co->p_flags &= ~PFENTER;
		}
	}
}

start_copilots(p)
register struct player	*p;
{
	register struct player	*co;
	register int		i;

	for (i = 0, co = &players[0]; i < MAXPLAYER; i++, co++) {
		if (co->p_status == PFREE)
			continue;
		if (co->p_ship == p->p_ship && co->p_copilot) {
			enter(co->p_ship->s_team, XDisplayString(co->display), co->p_no);
			co->p_status = PALIVE;
			co->p_flags &= ~PFENTER;
		}
	}
}

keyaction(p, key, data)
struct player	*p;
char key;
XKeyEvent *data;
{
    char buf[80];
    unsigned char course;
    struct obtype *gettarget(), *target;
    struct player *p2;
    struct planet *pl;

    switch (key) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    set_speed(p, key - '0' + p->p_ship->s_Ten);
	    break;
	case 'X':
	    p->p_ship->s_Ten += 10;
	    break;
	case 'k': /* k = set course */
	    course = getcourse(p, data->window, data->x, data->y);
	    set_course(p, course);
	    p->p_ship->s_flags &= ~(SFSLOCK | SFPLLOCK);
	    break;
	case 'p': /* p = fire phasers */
	    course = getcourse(p, data->window, data->x, data->y);
	    phaser(p, course);
	    break;
	case 't': /* t = launch torps */
	    course = getcourse(p, data->window, data->x, data->y);
	    ntorp(p, course, isGod(p) ? TSTRAIGHT : TMOVE);
	    break;
	case 'd': /* d = detonate other torps */
	    detothers(p);
	    break;
	case 'D': /* D = detonate my torps */
	    detmine(p);
	    break;
	case '+': /* + = Put shields up */
	    shield_up(p);
	    break;
	case '-': /* - = Put shields down */
	    shield_down(p);
	    break;
	case 'u': /* u = toggle shields */
	    shield_tog(p);
	    break;
	case 'b': /* b = bomb planet */
	    bomb_planet(p);
	    break;
	case 'z': /* z = beam up */
	    beam_up(p);
	    break;
	case 'x': /* x = beam down */
	    beam_down(p);
	    break;
	case 'R': /* R = Go into repair mode */
	    p->p_ship->s_flags &= ~(SFSLOCK | SFPLLOCK);
	    repair(p);
	    break;
	case 'o': /* o = orbit nearest planet */
	    p->p_ship->s_flags &= ~(SFSLOCK | SFPLLOCK);
	    orbit(p);
	    break;
	case 'Q':
	    if (p->p_copilot) {
		p->p_copilot = 0;
		calcstats(p);
		savestats(p);
		playerchange = 1;
		XCloseDisplay(p->display);
		p->display = (Display *) NULL;
		p->p_ship->s_numpilots--;
		p->p_flags &= ~PFENTER;
		p->p_status = PFREE;
		break;
	    }
	    if (isGod(p)) {
			p->p_ship->s_explode = 1;
			p->p_ship->s_whydead = KQUIT;
			p->p_ship->s_status = EXPLODE;
			p->p_status = PDEAD;
	    } else {
		    p->p_ship->s_flags |= SFSELFDEST;
		    p->p_ship->s_selfdest = p->p_ship->s_updates + PSELFDESTTIME;
		    warning(p, "Self destruct initiated");
	    }
	    break;
	case 'Z':
		if (isGod(p)) {
		    target = gettarget(p, data->window, data->x, data->y,
			TARG_PLAYER);
		    if (target->o_type == PLAYERTYPE) {
			p2 = &players[target->o_num];
			if (!isGod(p2)) {
				p2->p_ship->s_status = EXPLODE;
				p2->p_ship->s_explode = PEXPTIME;
				p2->p_ship->s_whodead = p->p_no;
				p2->p_ship->s_whydead = KLIGHTNING;
			}
		    } else {
			sprintf(buf, "No players in range.");
			warning(p, buf);
		    }
		} else
		    XBell(p->display, 0);
		break;
	case '?': /* ? = Redisplay all messages */
	    repeat_message(p);
	    break;
	case '/': /* / = Stop redisplay all message. */
	    p->p_lastm = mctl->mc_current;
	    break;
	case 'c': /* c = cloak */
	    if (p->p_ship->s_flags & SFTOWING) {
		warning(p, "You can't use your Tractor Beams and Cloak at the same time.");
		break;
	    }
	    cloak(p);
	    break;
	case 'C': /* C = coups */
	    coup(p);
	    break;
	case 'l': /* l = lock onto */
	    /* since a robot would never use this function (it's user
	       Interface dependent,) all the work is done here instead
	       of in interface.c */
	    target = gettarget(p, data->window, data->x, data->y,
		TARG_PLAYER|TARG_PLANET);
	    if (target->o_type == PLAYERTYPE) {
		p->p_ship->s_flags |= SFSLOCK;
		p->p_ship->s_flags &= ~(SFPLLOCK|SFORBIT|SFBEAMUP|SFBEAMDOWN|SFBOMB);
		p->p_ship->s_shipl = target->o_num;
		p2 = &players[target->o_num];
		sprintf(buf, "Locking onto %s (%c%d)",
		    p2->p_name,
		    teamlet[p2->p_ship->s_team],
		    p2->p_ship->s_no);
		warning(p, buf);
	    }
	    else { 	/* It's a planet */
		p->p_ship->s_flags |= SFPLLOCK;
		p->p_ship->s_flags &= ~(SFSLOCK|SFORBIT|SFBEAMUP|SFBEAMDOWN|SFBOMB);
		p->p_ship->s_planet = target->o_num;
		pl = &planets[target->o_num];
		sprintf(buf, "Locking onto %s",
		    pl->pl_name);
		warning(p, buf);
	    }
	    break;
	case '@': /* @ = toggle copilot permissions */
	    p->p_flags ^= PFCOPILOT;
	    break;
	case '*': /* * = send in practice robot */
	    /* Only if no other players on OTHER teams. */
	    if (isGod(p))
		    startrobot((random() % NUMTEAM) + 1, PFRHOSTILE|PFPRACTICER);
	    else if (tcount[p->p_ship->s_team] - (nships - nrobots) == 0)
		    startrobot(p->p_ship->s_team, PFRHOSTILE|PFPRACTICER);
	    break;
	case '&': /* & = send in harder robot */
	    /* Only if no other players on OTHER teams. */
	    if (isGod(p))
		    startrobot((random() % NUMTEAM) + 1, PFRHARD|PFRHOSTILE|PFPRACTICER);
	    else if (tcount[p->p_ship->s_team] - (nships - nrobots) == 0)
		    startrobot(p->p_ship->s_team, PFRHARD|PFRHOSTILE|PFPRACTICER);
	    break;
	case '^': /* & = send in very hard robot */
	    /* Only if no other players on OTHER teams. */
	    if (isGod(p))
		    startrobot((random() % NUMTEAM) + 1, PFRVHARD|PFRHOSTILE|PFPRACTICER);
	    else if (tcount[p->p_ship->s_team] - (nships - nrobots) == 0)
		    startrobot(p->p_ship->s_team, PFRVHARD|PFRHOSTILE|PFPRACTICER);
	    break;

	/* Start of display functions */
	case ' ': /* ' ' = clear special windows */
	    if (ismapped(p, p->playerw))
		XUnmapWindow(p->display, p->playerw);
	    if (ismapped(p, p->planetw))
		XUnmapWindow(p->display, p->planetw);
	    if (p->p_infomapped)
		destroyInfo(p);
	    if (ismapped(p, p->war))
		XUnmapWindow(p->display, p->war);
	    break;
	case 'L': /* L = Player list */
	    if (ismapped(p, p->playerw)) {
		XUnmapWindow(p->display, p->playerw);
	    } else {
		XMapWindow(p->display, p->playerw);
	    }
	    break;
	case 'P': /* P = Planet list */
	    if (ismapped(p, p->planetw)) {
		XUnmapWindow(p->display, p->planetw);
	    } else {
		XMapWindow(p->display, p->planetw);
	    }
	    break;
	case 'S': /* S = Score list */
	    if (p->p_infomapped)
		destroyInfo(p);
	    scorelist(p, 0);
	    break;
	case 's': /* s = toggle stat mode */
	   if (p->p_flags & PFSHOWSTATS) {
		closeStats(p, p->statwin);
	   } else {
		p->statwin = openStats(p);
	   }
	   break;
	case 'U': /* U = toggle show shields */
	   if (p->p_flags & PFSHOWSHIELDS) {
		p->p_flags &= ~PFSHOWSHIELDS;
	   } else {
		p->p_flags |= PFSHOWSHIELDS;
	   }
	   break;
	case 'M': /* M = Toggle Map mode */
	    if ((p->p_flags & PFNOMAPMODE) == 0)
		    p->p_mapmode = !p->p_mapmode;
	    break;
	case 'N': /* N = Toggle Name mode */
	    p->p_namemode = !p->p_namemode;
	    break;
	case 'i': /* i = get information */
	    if (!p->p_infomapped)
		inform(p, data->window, data->x, data->y);
	    else
		destroyInfo(p);
	    break;
	case 'h': /* h = Map help window */
	    if (ismapped(p, p->helpWin)) {
		XUnmapWindow(p->display, p->helpWin);
	    } else {
		XMapWindow(p->display, p->helpWin);
	    }
	    break;
	case 'w': /* w = map war stuff */
	    if (p->p_copilot) {
		warning(p, "Copilots cannot alter war settings");
		break;
	    }
	    if (ismapped(p, p->war)) {
		XUnmapWindow(p->display, p->war);
		p->p_redrawall = 1;
	    } else
		warwindow(p);
	    break;
	case 'T':  /* Try to tow someone (toggle switch). */
	    if (p->p_ship->s_flags & SFTOWING) {
		tow_off(p);
		break;
	    }
	    if (p->p_ship->s_flags & SFCLOAK) {
		warning(p, "You can't Cloak and use your Tractor Beams at the same time.");
		break;
	    }
	    target = gettarget(p, data->window, data->x, data->y,
		isGod(p) ? (TARG_PLAYER|TARG_PLANET) : TARG_PLAYER);
	    if (!isGod(p) && target->o_type != PLAYERTYPE) {
		/* Trying to tow a planet (or sun, or moon) */
		warning(p, "Don't be ridiculous.");
		break;
	    }
	    if (target->o_type == PLAYERTYPE && target->o_num == p->p_ship->s_no) {
		warning(p, "No ships found.");
		break;
	    }
	    tow_on(p, target);
	    break;
	default:
		fprintf(stderr, "Got unknown key 0x%x\n", key);
	    XBell(p->display, 0);
	    break;
    }
    if (key != 'X')
	p->p_ship->s_Ten = 0;
}

buttonaction(p, data)
register struct player	*p;
XButtonEvent *data;
{
    unsigned char course;

    if ((data->button & Button3) == Button3) {
	course = getcourse(p, data->window, data->x, data->y);
	p->p_ship->s_desdir = course;
	p->p_ship->s_flags &= ~(SFSLOCK | SFPLLOCK | SFORBIT);
    }
    else if ((data->button & Button1) == Button1) {
	course = getcourse(p, data->window, data->x, data->y);
	ntorp(p, course, isGod(p) ? TSTRAIGHT : TMOVE);
    }
    else if ((data->button & Button2) == Button2) {
	course = getcourse(p, data->window, data->x, data->y);
	phaser(p, course);
    }
}

getcourse(p, ww, x, y)
register struct player	*p;
Window ww;
int x, y;
{
    unsigned char	iatan2();

    if (ww == p->mapw) {
	int	me_x, me_y;

	me_x = p->p_ship->s_x * p->p_xwinsize / GWIDTH;
	me_y = p->p_ship->s_y * p->p_ywinsize / GWIDTH;
	return(iatan2(x - me_x, me_y - y));
    }
    else
	return(iatan2(x - p->p_xwinsize / 2, p->p_ywinsize / 2 - y));
}

inputIgnored(p)
register struct player	*p;
{
	if (p->p_status != PALIVE)
	    return (1);
	if (p->p_ship->s_flags & SFWAR) {
	    warning(p, "Battle computers being re-programmed");
	    return (1);
	}
	return (0);
}
