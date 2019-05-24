#include "mgr.h"
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

fd_set	listenset;	/* set of fd's on which we are listening */
Service *svchead;	/* head of services */
Service *nsvchead;	/* new head of services */

/*
 *  Parse a service definition line.  The line is of the form:
 *  `service action+action+action+...'
 */
Service *
newservice(cp)
	char *cp;
{
#	define MAXACTS 32
	char *arg;
	char *acts[MAXACTS];
	Service *sp=(Service *)malloc(sizeof(Service));
	int i, n;
	Action *lap;

	if(sp==NULL) {
		logevent("out of memory parsing service\n");
		return NULL;
	}
	sp->ap = NULL;
	sp->listen = -1;
	sp->name = NULL;
	sp->next = (Service *)NULL;
	sp->accept = 0;
	sp->lasttime = 1;

	/* find service name */
	for(; isspace(*cp); cp++)
		;
	for(arg=cp; *arg && !isspace(*arg); arg++)
		;
	if(isspace(*arg))
		*arg++ = '\0';
	sp->name = strdup(cp);

	/* separate actions */
	for(; isspace(*arg); arg++)
		;
	for(n=0; n<MAXACTS; n++) {
		if(*arg=='\0')
			break;
		acts[n] = arg;
		for(;;arg++)
			if(*arg=='\\' && *(arg+1)=='+') {
				arg++;
			} else if(*arg=='+') {
				*arg++ = '\0';
				break;
			} else if(*arg=='\0')
				break;
		for(; isspace(*arg)||*arg=='+'; arg++)
			;
	}
	if (n <= 0) {
		logevent("service with no action `%s'\n", cp);
		freeservice(sp);
		return NULL;
	}

	/* parse actions */
	for(lap=NULL, i=0; i<n; i++) {
		if (lap==NULL)
			sp->ap = lap = newaction(acts[i]);
		else
			lap = lap->next = newaction(acts[i]);
		if (lap==NULL) {
			freeservice(sp);
			return NULL;
		}
		lap->next = NULL;
		sp->accept |= lap->accept;
	}
	logevent("newservice(%s)\n", sp->name);
	return sp;
}

freeservice(sp)
	Service *sp;
{
	if(sp==NULL)
		return;
	if(sp->listen>=0) {
		logevent("denouncing %s\n", sp->name);
		close(sp->listen);
		FD_CLR(sp->listen, listenset);
	}
	if(sp->name!=NULL)
		free(sp->name);
	for(; sp->ap!=NULL; sp->ap=sp->ap->next)
		freeaction(sp->ap);
	free((char *)sp);
}

/*
 *  Add a service to the ones for which we are listening
 */
addservice(sp)
	Service *sp;
{
	Service *p;

	/* look for an announced version of the service */
	for(p=svchead; p; p=p->next)
		if (strcmp(sp->name, p->name)==0)
			break;

	/* inherit fd from old service */
	if (p) {
		sp->listen = p->listen;
		p->listen = -1;
	}

	/* add the new service */
	sp->next = nsvchead;
	nsvchead = sp;
	return 0;
}

/*
 *  Start the listening process on any services not already listening.
 */
startsvcs()
{
	Service *p, *np;

	/* denounce old services */
	for(p=svchead; p; p=np) {
		np = p->next;
		freeservice(p);
	}

	/* install new services */
	svchead = nsvchead;
	nsvchead = (Service *)NULL;

	announcesvcs();
}

/*
 *  Announce any services not already listening
 */
announcesvcs()
{
	Service *p;

	/* announce new services */
	for(p=svchead; p; p=p->next) {
		if (p->listen>=0)
			continue;
		logevent("announcing %s\n", p->name);
		p->listen = ipccreat(p->name, "light");
		if (p->listen<0) {
			logevent("failed\n");
			continue;
		}
		chmod(p->name, 0666);
		FD_SET(p->listen, listenset);
	}
}

/*
 *  Reset all services
 */
resetsvcs()
{
	Service *p, *np;

	/* shut down all listeners */
	logevent("resetsrvcs()\n");
	for(p=svchead; p; p=np) {
		logevent("retracting %s\n", p->name);
		np = p->next;
		freeservice(p);
	}
	svchead = (Service *)NULL;
	readfiles();
}

/*
 *  Get a request and vector to the appropriate service
 */
Request *
listen()
{
	fd_set readset;
	Service *sp;
	static Request rp;
	ipcinfo *ip;
	int n;

	for(;;) {
		readset = listenset;
		if((n=select(NOFILE, &readset, (fd_set *)NULL, 30*1000))<0) {
			if (errno!=EINTR) {
				logevent("select failed: %d\n", errno);
				resetsvcs();
			}
			continue;
		}
		if(checkfiles())
			readfiles();
		announcesvcs();
		if(n==0)
			continue;
		for(sp=svchead; sp; sp=sp->next)
			if (FD_ISSET(sp->listen, readset))
				break;
		if (!sp) {
			logevent("listen on bad fd\n");
			resetsvcs();
			continue;
		}
		if ((ip = ipclisten(sp->listen)) == NULL) {
			logevent("bad listen: fd %d\n", sp->listen);
			close(sp->listen);
			FD_CLR(sp->listen, listenset);
			sp->listen = -1;
			continue;
		}
		logevent("call for %s\n", ip->name);
		if (ip->machine==NULL || ip->user==NULL) {
			logevent("null machine/user called %s!\n", sp->name);
			ipcreject(ip, EACCES, "no machine or user name");
			continue;
		}
		logevent("call from %s!%s\n", ip->machine, ip->user);
		if ((!sp->accept) && ipcaccept(ip)<0) {
			logevent("can't accept %s from %s!%s\n", sp->name, ip->machine, ip->user);
			continue;
		}
		logevent("accept %s from %s %s\n", sp->name, ip->machine, ip->user);
		return(newrequest(ip, sp));
	}
}
