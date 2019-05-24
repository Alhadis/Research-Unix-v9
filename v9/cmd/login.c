/*
 * login  [ -f  name ] [ -p passwdline ] [ command ]
 *    -f:  if su, log in with no password
 *    -p:  if su, use entire password line.
 *    command: if given, just execute command
 */

#include <sys/param.h>
#include <sys/ttyio.h>
#include <utmp.h>
#include <signal.h>
#include <setjmp.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/lnode.h>
#include <sys/share.h>
#include <shares.h>
#include <errno.h>

#define	DEFSHARES	1	/* Default number of shares for a group 0 user */
#define	SYSERROR	(-1)
#define	MINUSAGE	1e6
#ifndef	ETOOMANYU
#define	ETOOMANYU	ENOSPC	/* Must go in errno.h or lnode.h */
#endif
#define SCPYN(a, b)	strncpy(a, b, sizeof(a))

#define ISIZE 32
#define POSTMKSIZ sizeof "From  Sun Jan 00 00:00:00 1979"
char	maildir[30] =	"/usr/spool/mail/";
struct	passwd nouser = {"", "nope"};
struct	utmp utmp;
char	minusnam[16] = "-";
char	homedir[64] = "HOME=";
char	path[] = "PATH=:/bin:/usr/bin";
char	**env;
int	nenv = 0;
char	nolog[] = "/etc/nologin";
struct	passwd *pwd;
struct	passwd *pwdecode();
char	*cmd;

struct	passwd *getpwnam();
char	*strcat();
int	setpwent();
char	*ttyname();
char	*crypt();
char	*getpass();
char	*strrchr(), *strchr();
extern	char **environ;

main(argc, argv)
char **argv;
{
	register char *namep;
	char input[ISIZE];
	char pwline[128];
	int t, f, c;
	char *ttyn;
	int neednopass = 0;
	int hangitup = 0;
	int ntries = 0;
	FILE *nlfd;
	int i;
	struct ttydevb tb;

	alarm(60);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	nice(-100);
	nice(20);
	nice(0);
#define	ONOFILE	20
	for (t=NSYSFILE; t<ONOFILE; t++)
		close(t);
	ttyn = ttyname(0);
	if (ttyn==0)
		ttyn = "/dev/tty??";
	SCPYN(input, "");
	switch(argc) {

	case 0:
	case 1:
		break;

	case 2:
		SCPYN(input, argv[1]);
		break;

	default:
		if (strcmp(argv[1], "-f")==0 || strcmp(argv[1], "-p")==0) {
			if (getuid()!=0) {
				printf("login: not super-user\n");
				exit(1);
			}
			neednopass++;
			if (strcmp(argv[1], "-f")==0)
				SCPYN(input, argv[2]);
			else {
				SCPYN(pwline, argv[2]);
				pwd = pwdecode(pwline);
				SCPYN(input, pwd->pw_name);
			}
			if (argc>3)
				cmd = argv[3];
		} else
			exit(1);
	}
    loop:
	if (ntries) {
		if (ntries > 5 || hangitup) {
			ioctl(0, TIOCGDEV, &tb);
			tb.ispeed = tb.ospeed = 0;
			ioctl(0, TIOCSDEV, &tb);
			sleep(5);
			exit(1);
		}
		neednopass = 0;
		pwd = NULL;
		SCPYN(input, "");
	}
	ntries++;
	while (input[0] == '\0') {
		namep = input;
		printf("login: ");
		while ((c = getchar()) != '\n') {
			if(c == ' ')
				c = '_';
			if (c == EOF)
				exit(0);
			if (namep < input + ISIZE - 1)
				*namep++ = c;
		}
		*namep = NULL;
	}
	SCPYN(utmp.ut_name, input);
	utmp.ut_time = 0;
	if (pwd == NULL) {
		setpwent();
		if ((pwd = getpwnam(input)) == NULL)
			pwd = &nouser;
		endpwent();
	}
	if (namep = strchr(utmp.ut_name, '\001'))
		if (namep[1]=='L' && namep[2]=='\002')	/* loopback? */
			hangitup++;
	time(&utmp.ut_time);
	SCPYN(utmp.ut_line, strchr(ttyn+1, '/')+1);
	if (*pwd->pw_passwd != '\0' && !neednopass) {
		namep = crypt(getpass("Password:"), pwd->pw_passwd);
		if (strcmp(namep, pwd->pw_passwd)) {
			/* magic string detects loopbacks */
			printf("\001L\002ogin incorrect\n");
			f = open("/usr/adm/xtmp", 1);
			if (f > 0) {
				lseek(f, 0L, 2);
				write(f, (char *)&utmp, sizeof(utmp));
				close(f);
			}
			goto loop;
		}
	}
	if(pwd->pw_uid != 0 && (nlfd = fopen(nolog, "r")) != NULL){
		while((c = getc(nlfd)) != EOF)
			putchar(c);
		exit(0);
	}
	if(setuplimits(pwd) < 0)
		goto loop;
	(void)setupgroups(pwd);
	if(chdir(pwd->pw_dir) < 0) {
		printf("No directory\n");
		if(pwd->pw_uid != 0 || (access(nolog, 0) < 0))
			goto loop;
	}
	setlogname(utmp.ut_name);
	if (cmd) {		/* remote exec */
		t = strlen(utmp.ut_name);
		if (t < sizeof(utmp.ut_name))
			utmp.ut_name[t] = '*';
	}
	t = ttyslot();
	if (t>0 && (f = open("/etc/utmp", 1)) >= 0) {
		lseek(f, (long)(t*sizeof(utmp)), 0);
		write(f, (char *)&utmp, sizeof(utmp));
		close(f);
	}
	if (t>0 && (f = open("/usr/adm/wtmp", 1)) >= 0) {
		lseek(f, 0L, 2);
		write(f, (char *)&utmp, sizeof(utmp));
		close(f);
	}
	chown(ttyn, pwd->pw_uid, pwd->pw_gid);
	chmod(ttyn, 0622);
	setgid(pwd->pw_gid);
	setuid(pwd->pw_uid);
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	nenv = 0;
	for(i = 0; environ[i]; i++)
		;
	env = (char **) malloc(sizeof(char *) * (i + 10));
	if (env == NULL) {
		printf("No memory for environment.\n");
		exit(1);
	}
	for (i = 0; environ[i]; i++) {
		if (strncmp(environ[i], "HOME=", 5) == 0)
			continue;
		if (strncmp(environ[i], "PATH=", 5) == 0)
			continue;
		env[nenv++] = environ[i];
	}
	if(homedir[0])
		env[nenv++] = homedir;
	if(path[0])
		env[nenv++] = path;
	env[nenv] = NULL;
	if ((namep = strrchr(pwd->pw_shell, '/')) == NULL)
		namep = pwd->pw_shell;
	else
		namep++;
	strcat(minusnam, namep);
	alarm(0);
	umask(02);
	if (cmd==NULL) {
		showmotd();
		strcat(maildir, pwd->pw_name);
		if(access(maildir,4)==0) {
			struct stat statb;
			stat(maildir, &statb);
			if (statb.st_size > POSTMKSIZ)
				printf("You have mail.\n");
		}
	}
	signal(SIGQUIT, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	environ = env;
	if (cmd==NULL)
		execlp(pwd->pw_shell, minusnam, 0);
	else {
		env[nenv++] = "REXEC=1";
		ioctl(0, TIOCEXCL, (void *)NULL);
		env[nenv] = 0;
		execlp(pwd->pw_shell, minusnam, "-c", cmd, (char *)0);
	}
	printf("No shell\n");
	exit(0);
}

int	stopmotd;
catch()
{
	signal(SIGINT, SIG_IGN);
	stopmotd++;
}

showmotd()
{
	FILE *mf;
	register c;

	signal(SIGINT, catch);
	if((mf = fopen("/etc/motd","r")) != NULL) {
		while((c = getc(mf)) != EOF && stopmotd == 0)
			putchar(c);
		fclose(mf);
	}
	signal(SIGINT, SIG_IGN);
}

/*
**	Set up parameters for share scheduler
*/

int	catchsys();
jmp_buf	Sigsysbuf;

int
setuplimits(pwd)
	register struct passwd *pwd;
{
	register int		(*oldsig)();
	register unsigned long	extime;
	struct sh_consts 	shconsts;
	struct lnode		share;

	if ( pwd->pw_uid == 0 )
		return 0;	/* root needs no set-up */

	oldsig = signal(SIGSYS, catchsys);

	if
	(
		setjmp(Sigsysbuf)
		||
		limits((struct lnode *)&shconsts, L_GETCOSTS) == SYSERROR
		||
		(Shareflags & NOSHARE)
	)
	{
		(void)signal(SIGSYS, oldsig);
		return 0;	/* Share not installed/active */
	}

	(void)signal(SIGSYS, oldsig);

	if ( (extime = getshares(&share, pwd->pw_uid, 0)) == 0 )
	{
		share.l_shares = DEFSHARES;
		share.l_usage = MINUSAGE;
	}
	else
	if ( limits(&share, L_OTHLIM) == SYSERROR )
	{
		/*
		**	Decay usage by time since last access.
		*/

		if ( (extime = (utmp.ut_time - extime) / Delta) > 0 )
		{
			extern double	pow();

			share.l_usage *= pow(DecayUsage, (float)extime);
			if ( share.l_usage < MINUSAGE )
				share.l_usage = MINUSAGE;
		}
	}

	if ( setlimits(&share) == SYSERROR )
	{
		if ( errno == ETOOMANYU )
		{
			char * cp = "other";

			share.l_uid = OTHERUID;

			if ( limits(&share, L_OTHLIM) != SYSERROR )
				(void)setlimits(&share);
			else
				cp = "root";

			printf("Warning: system out of share structures, using \"%s\".\n", cp);
		}
		else
			perror("setlimits");
	}

	closeshares();

	return 0;
}

int
catchsys(sig)
{
	longjmp(Sigsysbuf, 1);
	perror("longjmp");
	abort();
}

/*
**	Set up access groups.
*/

int
setupgroups(pwd)
	register struct passwd *pwd;
{
	register char **	cpp;
	register short *	gp;
	register struct group *	grp;
	register int		n;
	short			groups[NGROUPS];

	if ( getgroups(NGROUPS, groups) == SYSERROR )
		return;	/* Not installed */

	(void)setgrent();

	gp = groups;

	while ( gp < &groups[NGROUPS] && (grp = getgrent()) != (struct group *)0 )
		for ( cpp = grp->gr_mem ; *cpp != (char *)0 ; cpp++ )
			if ( strcmp(*cpp, pwd->pw_name) == 0 && grp->gr_gid != pwd->pw_gid )
			{
				*gp++ = grp->gr_gid;
				break;
			}

	(void)endgrent();

	if ( (n = gp-groups) == 0 )
		return;

	if ( setgroups(n, groups) == SYSERROR )
		perror("setgroups");
}
