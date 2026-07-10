static char sccsid[] = "@(#)xtrek.c	3.1";
#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#ifdef SUN40
#if !defined(sgi)
#include <sys/filio.h>
#endif
#else
#if !defined(cray)
# define	FD_SET(n, s)	(((s)->fds_bits[0]) |= (1 << n))
# define	FD_CLR(n, s)	(((s)->fds_bits[0]) &= ~(1 << n))
# define	FD_ZERO(s)	bzero((char *)(s), sizeof (*(s)))
# define	FD_ISSET(n, s)	(((s)->fds_bits[0]) & (1 << n))
#endif
#endif /* SUN40 */
#include <setjmp.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>

struct sockaddr_in	sin;
struct sockaddr_in	udp_sin;
struct servent		*sp;
jmp_buf			sdied;

done()
{
	fprintf(stderr, "broken pipe\n");
	longjmp(sdied);
}

main(ac, av)
register int	ac;
register char	**av;
{
	register struct hostent	*host = (struct hostent *) NULL;
	struct passwd	*pwent;
	int	s;
	char	hostname[80], *p;
	char	*h = (char *) NULL;	/* Ptr to server mach name */
	char	*display = (char *) NULL;	/* Ptr to X display name */
	char	*shipname = "ship";	/* Default ship name for in.xtrekd */
	int	qmode;
	int	rval;
	int	copilot;
	struct servent	*sp;
	extern struct servent	*getservbyname();
	extern char	*getenv();
	extern struct passwd	*getpwuid();
	/* jas (Jeff Schmidt)  For getopt() command line parsing. */
	char	c;
	extern int	optind, opterr;
	extern char	*optarg;

	copilot = 0xFF;
	qmode = 0;
	p = strrchr(av[0], '/');
	if (p == (char *) NULL)
		p = av[0];		/* No / found, use av[0] */
	else
		p++;			/* Use string after / */

	/* jas (Jeff Schmidt)  Check for the command line args. */

	while ((c = getopt(ac, av, "c:d:n:s:qSPR")) != EOF)
		switch (c) {
		  case 'c':			/* wants to copilot */
		     copilot = strtol(optarg, (char **) NULL, 16);
		     break;
		  case 'n':			/* Shipname to look for in .Xdefaults */
		     shipname = optarg;	break;
		  case 'd':			/* X display name follows */
		     display = optarg;	break;
		  case 's':			/* Xtrek server follows */
		     h = optarg;	break;
		  case 'q':			/* Query (biff) mode */
		     qmode = 1;		break;
		  case 'R':			/* Running Score? */
		     qmode = 2;		break;
		  case 'P':			/* Players? */
		     qmode = 3;		break;
		  case 'S':			/* List total Scores */
		     qmode = 4;		break;
		  case '?':			/* getopt() error */
		     usage(p);		break;
		} /* end switch (c) */

	pwent = getpwuid(getuid());
	if (pwent == (struct passwd *) NULL) {
		printf("Who are you?\n");
		exit(-1);
	}
	if (display == (char *) NULL)
		display = getenv("DISPLAY");

	if (!qmode && display == (char *) NULL) {
		printf("Your DISPLAY environment variable is not set.\n");
		exit(-1);
	}

/*
 * jas (Jeff Schmidt)  If xtrek server name was not given on the command
 *  line, then use the XTREKSERVER env variable, or default to "xtrek-server".
 */
	if (h == (char *) NULL) {
		h = getenv("XTREKSERVER");
		if (h == (char *) NULL)
			h = "xtrek-server";
	}
	sin.sin_addr.s_addr = inet_addr(h);
	if (sin.sin_addr.s_addr != -1) {
		sin.sin_family = AF_INET;
		(void) strcpy(hostname, h);
	} else {
		host = gethostbyname(h);
		if (host) {
			sin.sin_family = host->h_addrtype;
			bcopy(host->h_addr, (caddr_t) &sin.sin_addr,
				host->h_length);
			strcpy(hostname, host->h_name, strlen(host->h_name));
		} else {
			printf("%s: unknown host\n", h);
			exit(-1);
		}
	}
	sin.sin_port = 5701;

	signal(SIGPIPE, done);
#if !defined(NEWINET)
	/* Do something to get inetd to start up in.xtrekd */
	sp = getservbyname("xtrek", "udp");
	if (sp == (struct servent *) NULL) {
		goto normal;
	}
	udp_sin = sin;
	udp_sin.sin_port = sp->s_port;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		fprintf(stderr, "Can't open xtrek datagram socket\n");
		perror("socket");
		exit(-1);
	}
	if (connect(s, (struct sockaddr *) &udp_sin, sizeof (udp_sin)) < 0) {
		perror("connect udp");
		(void) close(s);
		exit(-1);
	}
	send(s, "WAKEUP!\015\012", 9, 0);
	close(s);
normal:
	sleep(5);		/* give the daemon a chance to start */
#endif
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket");
		exit(-1);
	}
	if (connect(s, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
		perror("connect");
		(void) close(s);
		exit(-1);
	}
	rval = 0;
	if (setjmp(sdied) == 0) {
		if (qmode)
			rval = query(s, qmode);
		else
			rval = startup(s, display, pwent->pw_name, shipname, copilot);
	}
	exit(rval);
}

query(s, mode)
register int	s, mode;
{
	char	buf[80];
	int	n = 1;
	int	tcount[4];
	int	rval;

	switch (mode) {
		case 2:		sprintf(buf, "RScore?");	break;
		case 3:		sprintf(buf, "Players?");	break;
		case 4:		sprintf(buf, "LScore?");	break;
		default:	sprintf(buf, "Query?");
	}
	write(s, buf, strlen(buf));
	write(s, "\015\012", 2);
	fflush(stdout);
	while ((n = read(s, buf, sizeof buf)) >= 0) {
		if (n <= 0) {
			write(s, "DONE\n", 5);	/* Does this cause SIGPIPE? */
			/* No, I think it causes read() above to return -1 */
			sleep(1);
		} else {
			if (mode == 1) {
				if (sscanf(buf, "F: %d, R: %d, K: %d, O: %d",
				 &tcount[0], &tcount[1], &tcount[2], &tcount[3]) != 4) {
					if (isatty(fileno(stdout)))
						printf("Bad Information\n");
					return(-1);
				} else {
					if (isatty(fileno(stdout)))
					   printf("Federation: %d, Romulan: %d, Klingon: %d, Orion: %d\n",
					    tcount[0], tcount[1], tcount[2], tcount[3]);
					rval = 0;
					if (tcount[0] > 0) rval += (1<<3);
					if (tcount[1] > 0) rval += (1<<2);
					if (tcount[2] > 0) rval += (1<<1);
					if (tcount[3] > 0) rval += (1<<0);
					return(rval);
				}
			} else {
				write(fileno(stdout), buf, n);
			}
		}
	}
	return 0;
}

startup(s, d, l, sn, copilot)
register int	s, copilot;
register char	*d, *l, *sn;
{
	char	buf[80];
	int	n = 1;

	sprintf(buf, "Display: %s Login: %s Shipname: %s Copilot: %x", d, l, sn, copilot);
	write(s, buf, strlen(buf));
	write(s, "\015\012", 2);
	fflush(stdout);
	while ((n = read(s, buf, sizeof buf)) >= 0) {
		if (n <= 0) {
			write(s, "DONE\n", 5);	/* Does this cause SIGPIPE? */
			/* No, I think it causes read() above to return -1 */
			sleep(1);
		} else
			write(fileno(stdout), buf, n);
	}
	return 0;
}

usage(p)
register char	*p;
{
	fprintf(stderr, "\nUsage: %s [-P|-q|-R|-S] [ -d display ] [ -s server ]\n\n", p);
	fprintf(stderr, " display defaults to DISPLAY env variable\n");
	fprintf(stderr, " server defaults to XTREKSERVER env variable or host 'xtrek-server'\n");
	fprintf(stderr, " -P  -> Query xtrek-server for current list of players\n");
	fprintf(stderr, " -q  -> Query xtrek-server for number of players\n");
	fprintf(stderr, " -R  -> Query xtrek-server for scores of presently playing players\n");
	fprintf(stderr, " -S  -> Query xtrek-server for a list of the Scorefile\n");
	exit(-1);
}
