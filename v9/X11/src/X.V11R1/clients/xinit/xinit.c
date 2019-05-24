#ifndef lint
static char *rcsid_xinit_c = "$Header: xinit.c,v 11.2 87/08/07 14:37:54 toddb Exp $";
#endif	lint
#include <X11/copyright.h>

/* Copyright    Massachusetts Institute of Technology    1986	*/

#include <X11/Xlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>		/* unused but resource.h needs it */
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>

#define	TRUE		1
#define	FALSE		0
#define	OK_EXIT		0
#define	ERR_EXIT	1
#define DEFAULT_SERVER "X"
#define DEFAULT_DISPLAY ":0"
char *default_client[] = {
	"xterm", "=+1+1", "-n", "login",
#ifdef sun
	"-C",
#endif
	"unix:0", NULL};
char *server[100];
char *client[100];
char *displayNum;
char *program;
Display *xd;			/* server connection */
union wait	status;
int serverpid = -1;
int clientpid = -1;
extern int	errno;

sigCatch(sig)
	int	sig;
{
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	Error("signal %d\n", sig);
	shutdown(serverpid, clientpid);
	exit(1);
}

main(argc, argv)
int argc;
register char **argv;
{
	register char **sptr = server;
	register char **cptr = client;
	register char **ptr;
	int pid, i;

	program = *argv++;
	argc--;

	/*
	 * copy the client args.
	 */
	if (argc == 0 || (**argv != '/' && !isalpha(**argv)))
		for (ptr = default_client; *ptr; )
			*cptr++ = *ptr++;
	while (argc && strcmp(*argv, "--")) {
		*cptr++ = *argv++;
		argc--;
	}
	*cptr = NULL;
	if (argc) {
		argv++;
		argc--;
	}

	/*
	 * Copy the server args.
	 */
	if (argc == 0 || (**argv != '/' && !isalpha(**argv))) {
		*sptr++ = DEFAULT_SERVER;
	} else {
		*sptr++ = *argv++;
		argc--;
	}
	if (argc > 0 && (argv[0][0] == ':' && isdigit(argv[0][1])))
		displayNum = *argv;
	else
		displayNum = *sptr++ = DEFAULT_DISPLAY;
	while (--argc >= 0)
		*sptr++ = *argv++;
	*sptr = NULL;

	/*
	 * Start the server and client.
	 */
	signal(SIGQUIT, sigCatch);
	signal(SIGINT, sigCatch);
	if ((serverpid = startServer(server)) > 0
	 && (clientpid = startClient(client)) > 0) {
		pid = -1;
		while (pid != clientpid && pid != serverpid)
			pid = wait(NULL);
	}
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	shutdown(serverpid, clientpid);

	if (serverpid < 0 || clientpid < 0)
		exit(ERR_EXIT);
	exit(OK_EXIT);
}


/*
 *	waitforserver - wait for X server to start up
 */

waitforserver(serverpid)
	int	serverpid;
{
	int	ncycles	 = 120;		/* # of cycles to wait */
	int	cycles;			/* Wait cycle count */
	char	display[100];		/* Display name */

	strcpy(display, "unix");
	strcat(display, displayNum);
	for (cycles = 0; cycles < ncycles; cycles++) {
		if (xd = XOpenDisplay(display)) {
			return(TRUE);
		}
		else {
			if (!processTimeout(serverpid, 1, "server to start"))
				return(FALSE);
		}
	}

	return(FALSE);
}

/*
 * return TRUE if we timeout waiting for pid to exit, FALSE otherwise.
 */
processTimeout(pid, timeout, string)
	int	pid, timeout;
	char	*string;
{
	int	i = 0, pidfound = -1;
	static char	*laststring;

	for (;;) {
		if ((pidfound = wait3(&status, WNOHANG, NULL)) == pid)
			break;
		if (timeout) {
			if (i == 0 && string != laststring)
				fprintf(stderr, "\nwaiting for %s ", string);
			else
				fprintf(stderr, ".", string);
			fflush(stderr);
		}
		if (timeout)
			sleep (1);
		if (++i > timeout)
			break;
	}
	laststring = string;
	return( pid != pidfound );
}

Error(fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9)
	char	*fmt;
{
	extern char	*sys_errlist[];

	fprintf(stderr, "%s: ", program);
	if (errno)
		fprintf(stderr, "%s: ", sys_errlist[ errno ]);
	fprintf(stderr, fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9);
}

Fatal(fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9)
	char	*fmt;
{
	Error(fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9);
	exit(ERR_EXIT);
}

startServer(server)
	char *server[];
{
	int	serverpid;

	serverpid = vfork();
	switch(serverpid) {
	case 0:
		close(0);
		close(1);

		/*
		 * prevent server from getting sighup from vhangup()
		 * if client is xterm -L
		 */
		setpgrp(0,0);

		execvp(server[0], server);
		Fatal("Server \"%s\" died on startup\n", server[0]);
		break;
	case -1:
		break;
	default:
		/*
		 * don't nice server
		 */
		setpriority( PRIO_PROCESS, serverpid, -1 );

		errno = 0;
		if (! processTimeout(serverpid, 0, "")) {
			serverpid = -1;
			break;
		}
		/*
		 * kludge to avoid race with TCP, giving server time to
		 * set his socket options before we try to open it
		 */
		sleep(5);

		if (waitforserver(serverpid) == 0) {
			Error("Can't connect to server\n");
			shutdown(serverpid, -1);
			serverpid = -1;
		}
		break;
	}

	return(serverpid);
}

startClient(client)
	char *client[];
{
	int	clientpid;

	if ((clientpid = vfork()) == 0) {
		setreuid(-1, -1);
		setpgrp(0, getpid());
		execvp(client[0], client);
		Fatal("Client \"%s\" died on startup\n", client[0]);
	}
	return (clientpid);
}

shutdown(serverpid, clientpid)
	int	serverpid, clientpid;
{
	/* have kept display opened, so close it now */
	if (clientpid > 0) {
		XCloseDisplay(xd);

		/* HUP all local clients to allow them to clean up */
		errno = 0;
		if (killpg(clientpid, SIGHUP) != 0)
			Error("can't killpg(%d, SIGHUP) for client\n",
				clientpid);
	}

	if (serverpid < 0)
		return;
	errno = 0;
	if (kill(serverpid, SIGTERM) < 0) {
		if (errno == EPERM)
			Fatal("Can't kill server\n");
		if (errno == ESRCH)
			return;
	}
	if (! processTimeout(serverpid, 10, "server to terminate"))
		return;

	fprintf(stderr, "timeout... send SIGKILL");
	fflush(stderr);
	errno = 0;
	if (kill(serverpid, SIGKILL) < 0) {
		if (errno == ESRCH)
			return;
	}
	if (processTimeout(serverpid, 3, "server to die"))
		Fatal("Can't kill server\n");
}
