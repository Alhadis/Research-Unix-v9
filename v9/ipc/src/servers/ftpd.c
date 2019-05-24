#ifndef lint
static char sccsid[] = "@(#)ftpd.c	4.28 (Berkeley) 9/22/83";
#endif

#define	CHOOSE_PORT
#define DEBUG
/*
 * FTP server.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <wait.h>

#include <sys/inet/in.h>
#include <sys/inet/tcp_user.h>

#include "ftp.h"

#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <setjmp.h>
#include <errno.h>

/*
 * File containing login names
 * NOT to be used on this machine.
 * Commonly used to disallow uucp.
 */
#define	FTPUSERS	"/etc/ftpusers"

extern	int errno;
extern	char *sys_errlist[];
extern	char *crypt();
extern	char version[];
extern	char *home;		/* pointer to home directory for glob */
extern	FILE *popen(), *fopen();
extern	int pclose(), fclose();

struct socket {
	unsigned short sport;
	long saddr;
} his_addr, data_dest, data_source, ctrl_addr;
#define	SO_KEEPALIVE	0x2

int	data;
jmp_buf	errcatch;
int	logged_in;
struct	passwd *pw;
int	debug;
int	timeout;
int	logging;
int	guest;
int	type;
int	form;
int	stru;			/* avoid C keyword */
int	mode;
int	usedefault = 1;		/* for data transfers */
char	hostname[32];
char	remotehost[32];
int	pasv;

/*
 * Timeout intervals for retrying connections
 * to hosts that don't accept PORT cmds.  This
 * is a kludge, but given the problems with TCP...
 */
#define	SWAITMAX	90	/* wait at most 90 seconds */
#define	SWAITINT	5	/* interval between retries */

int	swaitmax = SWAITMAX;
int	swaitint = SWAITINT;

int	lostconn();
int	reapchild();
FILE 	*getdatasock();
FILE	*dataconn();

main(argc, argv)
	int argc;
	char *argv[];
{
	int ctrl, s, options = 0;
	char *cp;
	int dev;
	int status;
	struct in_service *servp;

	servp = in_service("ftp", "tcp", 0);
	if (servp == 0) {
		fprintf(stderr, "ftpd: ftp/tcp: unknown service\n");
		exit(1);
	}
	ctrl_addr.sport = servp->port;
	data_source.sport = servp->port - 1;

	debug = 0;
	argc--, argv++;
	while (argc > 0 && *argv[0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'v':
			debug = 1;
			break;

		case 'd':
			debug = 1;
			break;

		case 'l':
			logging = 1;
			break;

		case 't':
			timeout = atoi(++cp);
			goto nextopt;
			break;

		default:
			fprintf(stderr, "Unknown flag -%c ignored.\n", *cp);
			break;
		}
nextopt:
		argc--, argv++;
	}

#ifndef DEBUG
	if (fork())
		exit(0);
	for (s = 0; s < 10; s++)
		if (!logging || (s != 2))
			(void) close(s);
	(void) open("/", 0);
	(void) dup2(0, 1);
	if (!logging)
		(void) dup2(0, 2);
	{ int tt = open("/dev/tty", 2);
	  if (tt > 0) {
/*		ioctl(tt, TIOCNOTTY, 0);	*/
		close(tt);
	  }
	}
#endif
	signal(SIGPIPE, SIG_IGN); /* used to be lostconn */
	signal(SIGHUP, SIG_IGN);

	fprintf(stderr,"ftpd started: options (debug,logging,timeout) are (%d,%d,%d)\n", debug, logging, timeout);
	do {
		struct tcpuser tu;

		s = tcp_sock();
		if (s < 0) {
			perror("ftpd: socket");;
			sleep(5);
			continue;
		}
		tu.lport = servp->port;
		tu.laddr = 0;
		tu.fport = 0;
		tu.faddr = 0;
		tu.param = SO_KEEPALIVE;
		if (tcp_listen(s, &tu) < 0) {
			close(s);
			s = -1;
			sleep(30);
			continue;
		}
	} while (s < 0);

	for (;;) {
		int ctrl;
		struct tcpuser tu;

		tu.lport = 0;
		tu.laddr = 0;
		tu.fport = 0;
		tu.faddr = 0;
		tu.param = 0;
		ctrl = tcp_accept(s, &tu);
		his_addr.saddr = tu.faddr;
		his_addr.sport = tu.fport;
		ctrl_addr.saddr = tu.laddr;
		ctrl_addr.sport = tu.lport;
		dev = tu.param;
		fprintf(stderr, "accept req from %s, port %d\n", inet_ntoa(his_addr.saddr), his_addr.sport);
		if (ctrl < 0) {
			if (errno == EINTR)
				continue;
			perror("ftpd: accept");
			sleep(1);
			continue;
		}
		while (wait3(&status, WNOHANG, 0) > 0)
			;
		signal(SIGCHLD, reapchild);
		switch(fork()) {
		case -1:
			printf("Out of processes\n");
			break;
		case 0:
			signal (SIGCHLD, SIG_IGN);
			dolog(&his_addr);
			close(s);
			dup2(ctrl, 0), close(ctrl), dup2(0, 1);
			/*
			 * Set up default state
			 */
			logged_in = 0;
			data = -1;
			type = TYPE_A;
			form = FORM_N;
			stru = STRU_F;
			mode = MODE_S;
/* try to do this equivalently
			(void) getsockname(0, &ctrl_addr, sizeof (ctrl_addr));
			gethostname(hostname, sizeof (hostname));
*/
			reply(220, "%s FTP server (%s) ready.",
				hostname, version);
			for (;;) {
				setjmp(errcatch);
				yyparse();
				fprintf(stderr, "out of yyparse\n");
			}
		}
		close(ctrl);
	}
}

reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, 0) > 0)
		;
}

lostconn()
{

	if (debug)
		fprintf(stderr, "Lost connection.\n");
	dologout(-1);
}

pass(passwd)
	char *passwd;
{
	char *xpasswd, *savestr();
	static struct passwd save;

	if (logged_in || pw == NULL) {
		reply(503, "Login with USER first.");
		return;
	}
	if (!guest) {		/* "ftp" is only account allowed no password */
		xpasswd = crypt(passwd, pw->pw_passwd);
		if (*pw->pw_passwd == '\0' || strcmp(xpasswd, pw->pw_passwd)) {
			reply(530, "Login incorrect.");
			pw = NULL;
			return;
		}
	}
/*
	setgid(pw->pw_gid);
	initgroups(pw->pw_name, pw->pw_gid);
*/
	if (chdir(pw->pw_dir)) {
		reply(550, "User %s: can't change directory to $s.",
			pw->pw_name, pw->pw_dir);
		goto bad;
	}
	if (guest && chroot(pw->pw_dir) < 0) {
		reply(550, "Can't set guest privileges.");
		goto bad;
	}
	if (!guest)
		reply(230, "User %s logged in.", pw->pw_name);
	else
		reply(230, "Guest login ok, access restrictions apply.");
	logged_in = 1;
	dologin(pw);
/*	setuid(pw->pw_uid); */
	/*
	 * Save everything so globbing doesn't
	 * clobber the fields.
	 */
	save = *pw;
	save.pw_name = savestr(pw->pw_name);
	save.pw_passwd = savestr(pw->pw_passwd);
	save.pw_comment = savestr(pw->pw_comment);
	save.pw_gecos = savestr(pw->pw_gecos, &save.pw_gecos);
	save.pw_dir = savestr(pw->pw_dir);
	save.pw_shell = savestr(pw->pw_shell);
	pw = &save;
	home = pw->pw_dir;		/* home dir for globbing */
	return;
bad:
/*	setuid(0); */
	pw = NULL;
}

char *
savestr(s)
	char *s;
{
	char *malloc();
	char *new = malloc(strlen(s) + 1);
	
	if (new != NULL)
		strcpy(new, s);
	return (new);
}


openfile(name)
char *name;
{
	reply(550, "%s: can't open", name);
}

retrieve(cmd, name)
	char *cmd, *name;
{
	FILE *fin, *dout;
	struct stat st;
	int (*closefunc)();

	if (cmd == 0) {
#ifdef notdef
		/* no remote command execution -- it's a security hole */
		if (*name == '|')
			fin = popen(name + 1, "r"), closefunc = pclose;
		else
#endif
		if (-1 == access2(name, 4, pw->pw_uid, pw->pw_gid)) {
			fprintf(stderr, "can't access %s\n", name);
			fin = NULL;
		} else
			fin = fopen(name, "r"), closefunc = fclose;
	} else {
		char line[BUFSIZ];

		sprintf(line, cmd, name), name = line;
		fprintf(stderr,"cmd is %s\n", line);
		fin = popen(line, "r"), closefunc = pclose;
	}
	if (fin == NULL) {
		fprintf(stderr, "bad popen()\n");
		if (errno != 0)
			reply(550, "%s: %s.", name, sys_errlist[errno]);
		return;
	}

	st.st_size = 0;
	if (cmd == 0 &&
	    (stat(name, &st) < 0 || (st.st_mode&S_IFMT) != S_IFREG)) {
		reply(550, "%s: not a plain file.", name);
		goto done;
	}
	dout = dataconn(name, st.st_size, "w");
	if (dout == NULL)
		goto done;
	if (send_data(fin, dout) || ferror(dout))
		reply(550, "%s: %s.", name, sys_errlist[errno]);

	else {
		fclose(dout);
		delay(timeout);
		reply(226, "Transfer complete.");
	}
	data = -1;
done:
	(*closefunc)(fin);
}

store(name, mode)
	char *name, *mode;
{
	FILE *fout, *din;
	int (*closefunc)(), dochown = 0;

#ifdef notdef
	/* no remote command execution -- it's a security hole */

	if (name[0] == '|')
		fout = popen(&name[1], "w"), closefunc = pclose;
	else
#endif
	{
		struct stat st;

		if (stat(name, &st) < 0)
			dochown++;
		if (-1 == access2(name, 2, pw->pw_uid, pw->pw_gid))
			fout = NULL;
		else
			fout = fopen(name, mode), closefunc = fclose;
	}
	if (fout == NULL) {
		reply(550, "%s: %s.", name, sys_errlist[errno]);
		return;
	}
	din = dataconn(name, (off_t)-1, "r");
	if (din == NULL)
		goto done;
	if (receive_data(din, fout) || ferror(fout))
		reply(550, "%s: %s.", name, sys_errlist[errno]);
	else {
		fflush(fout);
		delay(timeout);
		reply(226, "Transfer complete.");
	}
	fclose(din);
	data = -1;
done:
	if (dochown)
		(void) chown(name, pw->pw_uid, -1);
	(*closefunc)(fout);
}
FILE *
getdatasock(mode)
	char *mode;
{
	int s;

	if (data >= 0)
		return (fdopen(data, mode));
 	data_source.saddr = ctrl_addr.saddr;
	s = tcp_sock();
	if (s < 0) {
		perror("ftpd: socket");
		return(NULL);
	}
	return (fdopen(s, mode));
}

FILE *
dataconn(name, size, mode)
	char *name;
	off_t size;
	char *mode;
{
	char sizebuf[32];
	FILE *file;
	int retry = 0;
	struct tcpuser tu;

	if (size >= 0)
		sprintf (sizebuf, " (%ld bytes)", size);
	else
		(void) strcpy(sizebuf, "");
	if (data >= 0) {
		reply(125, "Using existing data connection for %s%s.",
		    name, sizebuf);
		usedefault = 1;
		return (fdopen(data, mode));
	}
	if (usedefault)
		data_dest = his_addr;
	usedefault = 1;
	file  = getdatasock(mode);
	if (file == NULL) {
		reply(425, "Can't create data socket (%s,%d): %s.",
			inet_ntoa(data_source.saddr),
			ntohs(data_source.sport),
		sys_errlist[errno]);
		return (NULL);
	}
	reply(150, "Opening data connection for %s (%s,%d)%s.",
	    name,
		inet_ntoa(data_dest.saddr),
		data_dest.sport, sizebuf);

	data = fileno(file);
	tu.lport = pasv?0:data_source.sport;
	tu.laddr = 0;
	tu.fport = data_dest.sport;
	tu.faddr = data_dest.saddr;
	tu.param = 0;
	while (tcp_connect(data, &tu) < 0) {		
		if (retry < swaitmax) {
			sleep(swaitint);
			retry += swaitint;
			continue;
		}
		reply(425, "Can't build data connection: %s.",
		    sys_errlist[errno]);
	       	(void)fclose(file);
		data = -1;
		return (NULL);
	}
	return (file);
}

/*
 * Tranfer the contents of "instr" to
 * "outstr" peer using the appropriate
 * encapulation of the date subject
 * to Mode, Structure, and Type.
 *
 * NB: Form isn't handled.
 */
send_data(instr, outstr)
	FILE *instr, *outstr;
{
	register int c;
	int netfd, filefd, cnt = 0;
	char buf[BUFSIZ];

	switch (type) {

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			if (c == '\n') {
				if (ferror (outstr))
					return (1);
				putc('\r', outstr);
			}
			putc(c, outstr);
			if (c == '\r')
				putc ('\0', outstr);
/*			cnt++;
			if (!(cnt%1000)) { fprintf(stderr, "%d ", cnt); fflush(stderr); } */
		}
		if (ferror (instr) || ferror (outstr)) {
			fprintf(stderr,"error: inst %d, outstr %d\n", ferror(instr), ferror(outstr));
			return (1);
		}
		return (0);
		
	case TYPE_I:
	case TYPE_L:
		netfd = fileno(outstr);
		filefd = fileno(instr);

		while ((cnt = read(filefd, buf, sizeof (buf))) > 0)
			if (write(netfd, buf, cnt) < 0) {
				fprintf(stderr, "write error, cnt=%d\n", cnt);
				return (1);
		}
		return (cnt < 0);
	}
	reply(504,"Unimplemented TYPE %d in send_data", type);
	return (1);
}

/*
 * Transfer data from peer to
 * "outstr" using the appropriate
 * encapulation of the data subject
 * to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled.
 */
receive_data(instr, outstr)
	FILE *instr, *outstr;
{
	register int c;
	int cnt;
	char buf[BUFSIZ];


	switch (type) {

	case TYPE_I:
	case TYPE_L:
		while ((cnt = read(fileno(instr), buf, sizeof buf)) > 0)
			if (write(fileno(outstr), buf, cnt) < 0)
				return (1);
		return (cnt < 0);

	case TYPE_E:
		reply(504, "TYPE E not implemented.");
		return (1);

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			if (c == '\r') {
				if (ferror (outstr))
					return (1);
				if ((c = getc(instr)) != '\n')
					putc ('\r', outstr);
				if (c == '\0')
					continue;
			}
			putc (c, outstr);
		}
		if (ferror (instr) || ferror (outstr))
			return (1);
		return (0);
	}
	fatal("Unknown type in receive_data.");
	/*NOTREACHED*/
}

fatal(s)
	char *s;
{
	reply(451, "Error in server: %s\n", s);
	reply(221, "Closing connection due to server error.");
	dologout(0);
}

reply(n, s, args)
	int n;
	char *s;
{

	printf("%d ", n);
	_doprnt(s, &args, stdout);
	printf("\r\n");
	fflush(stdout);
	if (debug) {
		fprintf(stderr, "<--- %d ", n);
		_doprnt(s, &args, stderr);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
}

lreply(n, s, args)
	int n;
	char *s;
{
	printf("%d-", n);
	_doprnt(s, &args, stdout);
	printf("\r\n");
	fflush(stdout);
	if (debug) {
		fprintf(stderr, "<--- %d-", n);
		_doprnt(s, &args, stderr);
		fprintf(stderr, "\n");
	}
}

replystr(s)
	char *s;
{
	printf("%s\r\n", s);
	fflush(stdout);
	if (debug)
		fprintf(stderr, "<--- %s\n", s);
}

ack(s)
	char *s;
{
	reply(200, "%s command okay.", s);
}

nack(s)
	char *s;
{
	reply(502, "%s command not implemented.", s);
}

yyerror()
{
	reply(500, "Command not understood.");
}

delete(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0) {
		reply(550, "%s: %s.", name, sys_errlist[errno]);
		return;
	}
	if ((st.st_mode&S_IFMT) == S_IFDIR) {
		if (rmdir(name) < 0) {
			reply(550, "%s: %s.", name, sys_errlist[errno]);
			return;
		}
		goto done;
	}
	if (unlink(name) < 0) {
		reply(550, "%s: %s.", name, sys_errlist[errno]);
		return;
	}
done:
	ack("DELE");
}

cwd(path)
	char *path;
{

	if (chdir(path) < 0) {
		reply(550, "%s: %s.", path, sys_errlist[errno]);
		return;
	}
	ack("CWD");
}

makedir(name)
	char *name;
{
	struct stat st;
	int dochown = stat(name, &st) < 0;

	if (mkdir(name, 0777) < 0) {
		reply(550, "%s: %s.", name, sys_errlist[errno]);
		return;
	}
	if (dochown)
		(void) chown(name, pw->pw_uid, -1);
	ack("MKDIR");
}

removedir(name)
	char *name;
{

	if (rmdir(name) < 0) {
		reply(550, "%s: %s.", name, sys_errlist[errno]);
		return;
	}
	ack("RMDIR");
}

#define	MAXPATHLEN	256
pwd()
{
	char path[MAXPATHLEN + 1];

	if (getwd(path) == NULL) {
		reply(451, "%s.", path);
		return;
	}
	reply(251, "\"%s\" is current directory.", path);
}

char *
renamefrom(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0) {
		reply(550, "%s: %s.", name, sys_errlist[errno]);
		return ((char *)0);
	}
	reply(350, "File exists, ready for destination name");
	return (name);
}

renamecmd(from, to)
	char *from, *to;
{

	if (rename(from, to) < 0) {
		reply(550, "rename: %s.", sys_errlist[errno]);
		return;
	}
	ack("RNTO");
}

dolog(addr)
	register struct socket *addr;
{
	char *h_name = (char *)in_host(addr->saddr);
	time_t	t;

	strncpy(remotehost, h_name, sizeof (remotehost));
	if (!logging)
		return;
	t = time(0);
	fprintf(stderr,"FTPD: connection from %s at %s", remotehost, ctime(&t));
	fflush(stderr);
}

#include <utmp.h>

#define	SCPYN(a, b)	strncpy(a, b, sizeof (a))
struct	utmp utmp;

/*
 * Record login in wtmp file.
 */

#define	O_WRONLY	1
#define O_APPEND	1
dologin(pw)
	struct passwd *pw;
{
	int wtmp;
	char line[32];

	pasv = 0;
	wtmp = open("/usr/adm/wtmp", O_WRONLY|O_APPEND);
	if (wtmp >= 0) {
		/* hack, but must be unique and no tty line */
		sprintf(line, "ftp%d", getpid());
		SCPYN(utmp.ut_line, line);
		SCPYN(utmp.ut_name, pw->pw_name);
		utmp.ut_time = time(0);
		(void) write(wtmp, (char *)&utmp, sizeof (utmp));
		(void) close(wtmp);
	}
}

/*
 * Record logout in wtmp file
 * and exit with supplied status.
 */
dologout(status)
	int status;
{
	int wtmp;

	if (!logged_in)
		_exit(status);
	wtmp = open("/usr/adm/wtmp", O_WRONLY|O_APPEND);
	if (wtmp >= 0) {
		SCPYN(utmp.ut_name, "");
		utmp.ut_time = time(0);
		(void) write(wtmp, (char *)&utmp, sizeof (utmp));
		(void) close(wtmp);
	}
	/* beware of flushing buffers after a SIGPIPE */
	_exit(status);
}

static char *
nextarg(cpp)
	char *cpp;
{
	register char *cp = cpp;

	if (cp == 0)
		return (cp);
	while (*cp && *cp != ' ' && *cp != '\t')
		cp++;
	if (*cp == ' ' || *cp == '\t') {
		*cp++ = '\0';
		while (*cp == ' ' || *cp == '\t')
			cp++;
	}
	if (cp == cpp)
		return ((char *)0);
	return (cp);
}

/*
 * Check user requesting login priviledges.
 * Disallow anyone mentioned in the file FTPUSERS
 * to allow people such as uucp to be avoided.
 */
checkuser(name)
	register char *name;
{
	char line[BUFSIZ], *strchr();
	FILE *fd;
	int found = 0;

	fd = fopen(FTPUSERS, "r");
	if (fd == NULL)
		return (1);
	while (fgets(line, sizeof (line), fd) != NULL) {
		register char *cp = strchr(line, '\n');

		if (cp)
			*cp = '\0';
		if (strcmp(line, name) == 0) {
			found++;
			break;
		}
	}
	fclose(fd);
	return (!found);
}
rename(from, to)
{
	if (-1 == link(from, to))
		return(-1);
	return(unlink(from));
}
inet_ntoa(addr)
long addr;
{
	static char b[32];
	unsigned char *xp = (unsigned char *)&addr;

	sprintf(b, "%d.%d.%d.%d", xp[3], xp[2], xp[1], xp[0]);
	return(b);
}
delay()
{
}
