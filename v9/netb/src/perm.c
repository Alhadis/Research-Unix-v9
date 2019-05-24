/* all the permission stuff. */
#include "share.h"
#include "pwd.h"
#include "grp.h"

typedef struct {
	int huid, cuid;
} tuid;
tuid *t, *g;
int tcnt, gcnt;
int primes[] = {23, 47, 97, 199, 397, 797, 1597, 3191, 6397, 12799, 31991, 0};
int *uhhash, *uchash, uhp, *ghhash, *gchash, ghp;
int otherok = 1;	/* default policy */
int pfd;	/* temporary file descriptor */
char *exbf, *realexbf;	/* exception buffer */
char clientname[128] = "\n";	/* the client's name, newline terminated */

clientuid()	/* from client to host */
{	int j;
	if(client.uid < 0)
		return(-1);
	for(j = client.uid % uhp; uchash[j] != -1; j++)
		if(t[uchash[j]].cuid == client.uid)
			return(t[uchash[j]].huid);
	return(-1);
}

clientgid()	/* from client to host */
{	int j;
	if(client.gid < 0)
		return(-1);
	for(j = client.gid % ghp; gchash[j] != -1; j++)
		if(g[gchash[j]].cuid == client.gid)
			return(g[gchash[j]].huid);
	return(-1);
}

hostuid(n)	/* host to client */
{	int j;
	if(n < 0)
		return(-1);
	for(j = n % uhp; uhhash[j] != -1; j++)
		if(t[uhhash[j]].huid == n)
			return(t[uhhash[j]].cuid);
	return(-1);
}

hostgid(n)	/* host to client */
{	int j;
	if(n < 0)
		return(-1);
	for(j = n % ghp; ghhash[j] != -1; j++)
		if(g[ghhash[j]].huid == n)
			return(g[ghhash[j]].cuid);
	return(-1);
}

isowner(p)
struct stat *p;
{
	if(clientuid() == p->st_uid)
		return(1);
	return(0);
}

searchperm()
{	struct stat stb;
	if(stat(nmbuf, &stb) < 0) {
		client.errno = errno;
		return(1);
	}
	if(clientuid() == stb.st_uid && (stb.st_mode & 0100))
		return(0);
	if(clientgid() == stb.st_gid && (stb.st_mode & 010))
		return(0);
	if((stb.st_mode & 1) && (otherok || clientuid() != -1))
		return(0);
	return(1);
}

writeperm()
{	struct stat stb;
	if(stat(nmbuf, &stb) < 0) {
		client.errno = errno;
		return(1);
	}
	if(clientuid() == stb.st_uid && (stb.st_mode & 0200))
		return(0);
	if(clientgid() == stb.st_gid && (stb.st_mode & 020))
		return(0);
	if((stb.st_mode & 2) && (otherok || clientuid() != -1))
		return(0);
	client.errno = EPERM;
	return(1);
}

dirwriteperm()
{	int n;
error("dw (%d,%d)\n", clientuid(), clientgid());
	if(!slash)
		return(1);	/* this can't happen? */
	if(clientuid() == -1 || clientgid() == -1) {
		client.errno = EPERM;
		return(1);	/* too bad bozo */
	}
	*slash = 0;
	n = writeperm();
	*slash = '/';
	return(n);
}

/* hostdev translates from host's devs to client's */
hostdev(n)
{	int i;
	for(i = 0; i < ndev; i++)
		if(devs[i].hdev == n)
			return(devs[i].cdev);
	error("hostdev: 0x%x not found\n", n);
	client.errno = ENXIO;
	return(0);
}

/* clientdev translates from client's to host's */
clientdev(n)
{	int i;
	for(i = 0; i < ndev; i++)
		if(devs[i].cdev == n)
			return(devs[i].hdev);
	debug("clientdev: 0x%x not found\n", n);
	client.errno = ENXIO;
	return(0);
}

getuids()
{	int i, j;
	char msg[16];
	/* scan host passwd file for count */
	tcnt = cntpasswd();
	t = (tuid *) malloc(tcnt * sizeof(tuid));
	i = xread(cfd, inbuf, inlen);
	if(i != inlen)
		fatal("getuids read %d (%d)\n", i, errno);
	/* decode and ack */
	msg[0] = 2;
	if(write(cfd, msg, 1) != 1)
		fatal("getuid ack failed (%d)\n", errno);
	/* and match with host passwd */
	fillpasswd();
	for(i = 0; primes[i] && primes[i] < 2 * tcnt; i++)
		;
	if(primes[i] == 0)
		fatal("tcnt %d too big for hashtable %d\n", tcnt, primes[i-1]);
	uhp = primes[i];
	uhhash = (int *) malloc((uhp + 4) * sizeof(int));
	uchash = (int *) malloc((uhp + 4) * sizeof(int));
	/* what's the chance the last 3 items will fill? */
	for(i = 0; i < uhp + 4; i++)
		uhhash[i] = uchash[i] = -1;
	for(i = 0; i < tcnt; i++) {
		j = t[i].huid % uhp;
		while(uhhash[j] != -1)
			j++;
		if(j >= uhp + 3)	/* guarantee sentinel at end of hash tables */
			fatal("uhhash overflow!\n");
		uhhash[j] = i;

		j = t[i].cuid % uhp;
		while(uchash[j] != -1)
			j++;
		if(j >= uhp + 3)
			fatal("uchash overflow!\n");
		uchash[j] = i;
	}
}

getgids()
{	int i, j;
	char msg[16];
	/* scan host group file for count */
	gcnt = cntgroups();
	g = (tuid *) malloc(gcnt * sizeof(tuid));
	i = xread(cfd, inbuf, inlen);
	if(i != inlen)
		fatal("getgids read %d (%d)\n", i, errno);
	/* decode and ack */
	msg[0] = 3;
	if(write(cfd, msg, 1) != 1)
		fatal("getgid ack failed (%d)\n", errno);
	/* and match with host passwd */
	fillgroups();
	for(i = 0; primes[i] && primes[i] < 2 * gcnt; i++)
		;
	if(primes[i] == 0)
		fatal("gcnt %d too big for hashtable %d\n", gcnt, primes[i-1]);
	ghp = primes[i];
	ghhash = (int *) malloc((ghp + 4) * sizeof(int));
	gchash = (int *) malloc((ghp + 4) * sizeof(int));
	/* what's the chance the last 3 items will fill? */
	for(i = 0; i < ghp + 4; i++)
		ghhash[i] = gchash[i] = -1;
	for(i = 0; i < gcnt; i++) {
		j = g[i].huid % ghp;
		while(ghhash[j] != -1)
			j++;
		if(j >= ghp + 3) /* guarantee sentinel at end of hash tables */
			fatal("ghhash overflow!\n");
		ghhash[j] = i;

		j = g[i].cuid % ghp;
		while(gchash[j] != -1)
			j++;
		if(j >= ghp + 3)
			fatal("gchash overflow!\n");
		gchash[j] = i;
	}
}

cntpasswd()
{	struct passwd *p, *getpwent();
	int m = 0;
	while(p = getpwent())
		m++;
	setpwent();	/* leave it set for fillpasswd() */
	return(m);
}

cntgroups()
{	struct group *g, *getgrent();
	int m = 0;
	while(g = getgrent()) {
		m++;
	}
	setgrent();	/* leave it set for fillgroups() */
	return(m);
}

char *
clientline(s)
char *s;
{	char *p;
	if(!s || !*s)
		return(0);
	for(p = s; *p; ) {
		if(strncmp(p, "client ", 7) == 0 || strncmp(p, "client\t", 7) == 0)
			return(p);
		while(*p++ != '\n');
			;
	}
	return(p);
}

getexcepts()
{	char *p, *q, *r;
	struct stat stb;
	pfd = open("/usr/netb/except", 0);
	if(pfd < 0 || fstat(pfd, &stb) < 0) {
		error("danger!!! can't find /usr/netb/except, but proceeding\n");
fakeit:
		exbf = (char *) malloc(1);
		realexbf = exbf;
		*exbf = 0;
		return;
	}
	exbf = (char *) malloc(stb.st_size + 1);
	realexbf = exbf;
	if(read(pfd, exbf, stb.st_size) != stb.st_size) {
		error("danger!!! can't read /usr/netb/except, but proceeding\n");
		free(exbf);
		goto fakeit;
	}
	close(pfd);
	/* now find the section for this client */
	for(q = 0, p = clientline(exbf); *p; p = clientline(p)) {
		if(q) {
			*p = 0;
			exbf = q;
			error("found excepts for client %s");
			return;
		}
		r = p + 6;	/* skip across "client " */
		while(*r == ' ' || *r == '\t')
			r++;
		if(strncmp(r, clientname, strlen(clientname)) == 0)
			q = p;
		while(*p++ != '\n')
			;
	}
	error("warning!!! no match for client %s", clientname);
	/* look for the default client */
	for(q = 0, p = clientline(exbf); *p && !q; p = clientline(p+1)) {
		r = p + 6;	/* skip across "client " */
		while(*r == ' ' || *r == '\t')
			r++;
		if(*r == '*' && r[1] == '\n')
			q = p;
	}
	if(q) {
		exbf = q;
		error("using default client info\n");
		return;
	}
	error("warning!!! no default client info, proceeding anyway\n");
}

char *
findexcept(tag, s)
char *tag, *s;
{	char *p, *q;
	int n;
	n = strlen(tag);
	p = exbf;
loop:
	if(*p == 0)
		return(s);
	if(strncmp(tag, p, n) != 0) {
eol:
		while(*p && *p++ != '\n')
			;
		goto loop;
	}
	for(p += n; *p == ' ' || *p == '\t'; p++)
		;	/* skip the tag and whitespace */
	for(q = s; *p == *q; p++, q++)
		;
	if(*q != 0 || *p != '=')
		goto eol;
	if(p[1] == ' ' || p[1] == '\t' || p[1] == '\n')
		return(0);	/* client not allowed */
	return(p+1);
}

doneexcepts()
{	char *s;
	s = findexcept("param", "otherok");
	if(s && *s == '0')
		otherok = 0;
	error("otherok %d\n", otherok);
	/* umask could be settable too */
	umask(0);
	free(realexbf);
	realexbf = exbf = (char *) -1;	/* dereference that */
}

char *
findid(t, s)
char *t, *s;
{	char *p, *q;
	s = findexcept(t, s);
	if(!s)
		return(0);
	p = (char *)inbuf;
loop:
	if(*p == 0)
		return(0);
	for(q = s; *p == *q; p++, q++)
		;
	if((*q == 0 || *q == ' ' || *q == '\t' || *q == '\n') && *p == ' ')
		return(p+1);
	while(*p && *p != '\n')
		p++;
	if(*p == '\n')
		p++;
	goto loop;
}

fillpasswd()	/* quadratic */
{	struct passwd *p, *getpwent();
	char *s;
	int i = 0;
	while(p = getpwent()) {
		s = findid("uid", p->pw_name);
		if(s == 0)
			continue;
		t[i].huid = p->pw_uid;
		t[i].cuid = atoi(s);
		i++;
	}
	error("matched %d passwds out of %d\n", i, tcnt);
	tcnt = i;
	endpwent();
}

fillgroups()	/* quadratic */
{	struct group *p, *getgrent();
	char *s;
	int i = 0;
	while(p = getgrent()) {
		s = findid("gid", p->gr_name);
		if(s == 0)
			continue;
		g[i].huid = p->gr_gid;
		g[i].cuid = atoi(s);
		i++;
	}
	error("matched %d groups out of %d\n", i, gcnt);
	gcnt = i;
	endgrent();
}
