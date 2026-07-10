static char sccsid[] = "@(#)main.c	3.1";
#ifndef lint
static char *rcsid_main_c = "$Header: /uraid2/riedl/src/xceltrek/RCS/main.c,v 1.1 88/04/18 16:10:25 riedl Exp Locker: riedl $";
#endif	lint
/* Copyright (c) 1986 	Chris Guthrie */

#include <X11/Xlib.h>
#include <stdio.h>
#if !defined(cray)
#include <sys/types.h>
#endif
#include <sys/time.h>
#include <sys/file.h>
#include <signal.h>
#include <pwd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#if defined(hpux) || defined(cray)
#include <fcntl.h>
#endif

#ifdef SUN40
#include <sys/filio.h>
#else
#if !defined(cray)
# define	FD_SET(n, s)	(((s)->fds_bits[0]) |= (1 << n))
# define	FD_CLR(n, s)	(((s)->fds_bits[0]) &= ~(1 << n))
# define	FD_ZERO(s)	bzero((char *)(s), sizeof (*(s)))
# define	FD_ISSET(n, s)	(((s)->fds_bits[0]) & (1 << n))
#endif
#include <sys/ioctl.h>
#endif /* SUN40 */

#include "defs.h"
#include "data.h"

extern int debug;
extern int	xtrek_socket;
struct sockaddr_in	xtrekAddress;

main(argc, argv)
int argc;
char **argv;
{
    struct player	*p;
    struct servent	*sp;
    extern struct servent	*getservbyname();
    int docycle(), move();
    char *host;
    register int i;
    extern char *optarg;
    extern int optind, opterr;
    char *getenv();
    char c;
    int argerr = 0;
    int	on = 1;
    int oldinetd = 0;
    long xxxtime;
    int	result;
    int	started_from_cmdline;

    started_from_cmdline = isatty(fileno(stdin));

#if !defined(NEWINET)
    oldinetd = 1;
#endif
    while ((c = getopt(argc, argv, "dl:")) != EOF) {
      switch (c) {
      case 'd':
	debug++;
	break;
      case 'l':
	strncpy(DIR, optarg, sizeof (DIR) - 1);
	break;
      case '?':
      default:
	argerr = 1;
	break;
      }
    }
    if (optind < argc) host = argv[optind];
    else host = getenv("DISPLAY");

    if (argerr) {
	printf("Usage: %s [-d] [-l lib_directory]\n",
	       argv[0]);
	exit(1);
    }

    chdir(DIR);

    if (!debug) {
	    close(fileno(stderr));
	    open("stderr.out", O_CREAT|O_RDWR|O_APPEND, 0666);
    }
    time(&xxxtime);
    fprintf(stderr, "in.xtrekd: starting at %s\n", ctime(&xxxtime));

    if (debug || oldinetd || started_from_cmdline) {
	    if (oldinetd) {
#ifdef notdef
		    sp = getservbyname("xtrek", "udp");
		    if (sp == (struct servent *) NULL) {
			fprintf(stderr, "Can't find xtrek udp service\n");
			exit(1);
		    }
#endif
		    if (!debug && !started_from_cmdline) {
			ioctl(0, FIONBIO, (char *) &on);
			(void) setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);
		    }
	    }
	    sp = getservbyname("xtrek", "tcp");
	    xtrekAddress.sin_family = AF_INET;
	    xtrekAddress.sin_addr.s_addr = INADDR_ANY;
	    xtrekAddress.sin_port = sp ? sp->s_port : DEF_TCP_PORT;

	    xtrek_socket = socket(AF_INET, SOCK_STREAM, 0);
	    if (xtrek_socket < 0) {
		fprintf(stderr, "Can't open xtrek socket\n");
		perror("socket");
		exit(1);
	    }
	    if (bind(xtrek_socket, &xtrekAddress, sizeof xtrekAddress) < 0) {
		fprintf(stderr, "Can't bind to xtrek address\n");
		perror("bind");
		(void) close(xtrek_socket);
		exit(1);
	    }
    } else {
	    xtrek_socket = 0;
    }
    (void) setsockopt(xtrek_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
    (void) setsockopt(xtrek_socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);
    ioctl(xtrek_socket, FIONBIO, (char *) &on);

    if (listen(xtrek_socket, MAXPLAYER) < 0) {
	fprintf(stderr, "Can't listen on xtrek socket\n");
	perror("listen");
	(void) close(xtrek_socket);
	exit(1);
    }

    /* this finds the shared memory information plus a player slot */
    initialize();

    /* The main loop monster! */
    input();
}

drain_udp_socket() {
	register int	cc;
	static char	rbuf[25];

	while (1) {
		if ((cc = recv(0, rbuf, sizeof rbuf, 0)) <= 0)
			break;
	}
}
