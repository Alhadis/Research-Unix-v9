#include "mgr.h"
#include <sys/param.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <ctype.h>
#include "defs.h"

/*
 *  get parameters from caller
 */
parms(rp, ap)
	Request *rp;
	Action *ap;
{
	char pbuf[ARB];
	char *s;
	char *rdline();
	char *strchr();

	USE(ap);
	if ((s = rdline(rp->i->cfd)) == NULL)
		return -1;
	rp->args = strdup(s);
	return 0;
}

/*
 *  get terminal type(speed, etc) from caller
 */
term(rp, ap)
	Request *rp;
	Action *ap;
{
	char pbuf[ARB];
	char *s, *sl;
	char *rdline();
	char *strchr();

	USE(ap);
	if (ap->arg!=NULL && *(ap->arg)!='\0') {
		rp->term = strdup(ap->arg);
	} else {
		if((s = rdline(rp->i->cfd)) == NULL)
			return -1;
		if(sl=strchr(s, '/'))
			*sl = '\0';
		rp->term = strdup(s);
	}
	return 0;
}

/*
 *  action routines that end in execs
 */

doconn(rp, ap)
	Request *rp;
	Action *ap;
{
	USE(ap);
	ioctl(rp->i->cfd, TCPIOHUP, 0);		/* hang-up TCP on FIN */
	login(rp, rp->line, (char *)NULL);
}

doexec(rp, ap)
	Request *rp;
	Action *ap;
{
	USE(ap);
	login(rp, rp->line, rp->args);
}

docmd(rp, ap)
	Request *rp;
	Action *ap;
{
	register char *p;
	char buf[ARB];

	for(p=rp->args; *p; p++)
		if (strchr("\n&;^|<>(){}`", *p))
			*p = ' ';
	strcpy(buf, ap->arg);
	strcat(buf, " ");
	/*
	for(p=rp->args; *p && !isspace(*p); p++)
		;
	 */
	strcat(buf, rp->args);
	login(rp, rp->line, buf);
}

login(rp, pw, cmd)
	Request *rp;
	char *pw, *cmd;
{
	char *args[5];
	register char **ap;
	register int i;
	char **inienv;
	char **srcenv(), **termenv();

	ap = args;
	*ap++ = "login";
	if (pw) {
		*ap++ = "-p";
		*ap++ = pw;
		if (cmd)
			*ap++ = cmd;
	}
	*ap = NULL;
	for (i = 0; i < NSYSFILE; i++)
		dup2(rp->i->cfd, i);
	for (; i < NOFILE; i++)
		close(i);
	termenv(rp);
	inienv = srcenv(rp);
	ioctl(0, TIOCSPGRP, 0);
	execve("/etc/login", args, inienv);
	execve("/bin/login", args, inienv);
	_exit(1);
}

char **newep;	/* set up by init */
char *newenv[5];

char **
srcenv(rp)
	Request *rp;
{
	static char buf[100];

	sprintf(buf, "CSOURCE=%s!%s", rp->i->machine, rp->i->user);
	*newep++ = buf;
	*newep = NULL;
	return (newenv);
}

char **
termenv(rp)
	Request *rp;
{
	static char buf[100];

	if (rp->term == NULL)
		return (newenv);
	sprintf(buf, "TERM=%s", rp->term);
	*newep++ = buf;
	*newep = NULL;
	return (newenv);
}
