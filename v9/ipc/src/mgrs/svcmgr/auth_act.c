#include "mgr.h"
#include <pwd.h>
#include <sys/ioctl.h>
#include "defs.h"

/*
 *  execute as a specific user
 */
asuser(rp, ap)
	Request *rp;
	Action *ap;
{
	char line[ARB];
	struct passwd *pwsearch();

	if(pwsearch(ap->arg, -1, line)==NULL) {
		logevent("bad login: %s\n", ap->arg);
		return -1;
	}
	rp->line = strdup(line);
	return 0;
}

/*
 *  simple authentication
 */
auth(rp, ap)
	Request *rp;
	Action *ap;
{
	struct passwd *pw;
	static char line[ARB];
	struct passwd *pwsearch();
	char *mapuser();
	char *u;

	USE(ap);
	/* do the mapping from the authentication files */
	u = mapuser(rp->s->name, rp->i->machine, rp->i->user);
	if(u!=NULL) {
		if ((pw = pwsearch(u, -1, line)) != NULL
		&&   strcmp(pw->pw_name, "root") != 0) {
			rp->line = line;
			return 0;
		}
	}
	return 1;
}

/*
 *  v9 authentication
 */
v9auth(rp, ap)
	Request *rp;
	Action *ap;
{
	struct passwd *pw;
	static char line[ARB];
	register char *u, *p;
	struct passwd *pwsearch();
	char *mapuser();
	char *rdline();

	USE(ap);
	/* do the mapping from the authentication files */
	u = mapuser(rp->s->name, rp->i->machine, rp->i->user);
	if(u!=NULL) {
		if ((pw = pwsearch(u, -1, line)) != NULL
		&&   pw->pw_uid != 0) {
			write(rp->i->cfd, "OK", 2);
			rp->line = line;
			return 0;
		}
	}
	for (;;) {
		write(rp->i->cfd, "NO", 2);
		if ((u = rdline(rp->i->cfd))==NULL)
			return -1;
		p = strchr(u, ',');
		if (p)
			*p++ = '\0';
		if ((pw = pwsearch(u, -1, line)) == NULL)
			continue;
		if (strcmp(crypt(p, pw->pw_passwd), pw->pw_passwd) == 0)
			break;
	}
	write(rp->i->cfd, "OK", 2);
	rp->line = strdup(line);
	return 0;
}

/* 4.2BSD inet stye authentication */
#define SNDMSG(x) write(rp->i->cfd, x, strlen(x))
inauth(rp, ap)
	Request *rp;
	Action *ap;
{
	static char line[ARB];
	struct passwd *pw;
	struct passwd *pwsearch();
	char *port;
	char *u;
	char buf[ARB];
	char *rdline();
	char *mapuser();
	char *tcptofs();

	USE(ap);
	/* get port number for stderr */
	port = rdline(rp->i->cfd);
	if(port==NULL) {
		SNDMSG("\nprotocol botch\n");
		return -1;
	}
	if(*port!='\0')
		rp->errfd = ipcopen(ipcpath(rp->i->machine, "tcp",
					tcptofs(atoi(port))), "light");

	/* get remuser, locuser */
	u = rdline(rp->i->cfd);
	if(u==NULL) {
		SNDMSG("\nprotocol botch\n");
		return -1;
	}
	strcpy(buf, u);
	u = rdline(rp->i->cfd);
	if(u==NULL) {
		SNDMSG("\nprotocol botch\n");
		return -1;
	}
	if(strcmp(buf, u)!=0) {
		SNDMSG("\ncannot specify user-id\n");
		return -1;
	}
	write(rp->i->cfd, "", 1);

	/* authenticate */
	if ((u=mapuser(rp->s->name, rp->i->machine, u))!=NULL) {
		if ((pw = pwsearch(u, -1, line)) != NULL
		&&   pw->pw_uid != 0)
			rp->line = strdup(line);
	}
	return 0;
}
