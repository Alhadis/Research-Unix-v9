#include "sam.h"
#include <sys/ioctl.h>
#ifdef SUN
/* a hack to get around redef of ushort in sys/types.h .
   this depends on non-use of ushort in this file */
#define ushort USHORT
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifndef SUN
#include "/usr/jerq/include/jioctl.h"
#endif

writef(f)
	register File *f;
{
	uchar c;
	Posn n;
	struct stat statb;
	int newfile=0;
	if(stat((char *)genstr.s, &statb)==-1)
		newfile++;
	else if(strcmp(genstr.s, f->name.s)==0 &&
	        (f->inumber!=statb.st_ino || f->date<statb.st_mtime)){
		f->inumber=statb.st_ino;
		f->date=statb.st_mtime;
		warn_s(Wdate, (char *)genstr.s);
		return;
	}
	if((io=creat((char *)genstr.s, 0666))<0)
		error_s(Ecreate, (char *)genstr.s);
	dprint("%s: ", (char *)genstr.s);
	n=writeio(f);
	if(f->name.s[0]==0 || strcmp(genstr.s, f->name.s)==0)
		state(f, addr.r.p1==0 && addr.r.p2==f->nbytes? Clean : Dirty);
	if(newfile)
		dprint("(new file) ");
	if(addr.r.p2>0 && Fchars(f, &c, addr.r.p2-1, addr.r.p2) && c!='\n')
		warn(Wnotnewline);
	if(f->name.s[0]==0 || strcmp(genstr.s, f->name.s)==0){
		fstat(io, &statb);
		f->inumber=statb.st_ino;
		f->date=statb.st_mtime;
	}
	closeio(n);
}
Posn
readio(f, nonascii, setdate)
	register File *f;
	int *nonascii;
{
	register n;
	register Posn nt;
	register Posn p=addr.r.p2;
	struct stat statb;
	if(nonascii)
		*nonascii=FALSE;
	for(nt=0; (n=read(io, (char *)genbuf, BLOCKSIZE))>0; nt+=n){
		n=clean(genbuf, n, nonascii);
		Finsert(f, tempstr(genbuf, n), p);
	}
	if(nonascii && *nonascii)
		warn(Wnonascii);
	if(setdate){
		fstat(io, &statb);
		f->inumber=statb.st_ino;
		f->date=statb.st_mtime;
	}
	return nt;
}
Posn
writeio(f)
	register File *f;
{
	register n;
	register Posn p=addr.r.p1;
	while(p<addr.r.p2){
		if(addr.r.p2-p>BLOCKSIZE)
			n=BLOCKSIZE;
		else
			n=addr.r.p2-p;
		if(Fchars(f, genbuf, p, p+n)!=n)
			panic("writef read");
		(void)Write(io, genbuf, n);
		p+=n;
	}
	return addr.r.p2-addr.r.p1;
}
closeio(p)
	Posn p;
{
	(void)close(io);
	io=0;
	if(p>=0)
		dprint("#%lud\n", p);
}
clean(s, n, nonascii)
	register uchar *s;
	register n;
	int *nonascii;
{
	register uchar *p, *q;
	register i;
	/* for speed, just look first */
	p=s;
	if(i=n)
		do
			if(*p==0 || (*p++&HIGHBIT))
				goto Cleanup;
		while(--i);
	return n;	/* usual case */
    Cleanup:
	for(q=p=s, i=0; i<n; i++, p++)
		if(*p!=0 && (*p&HIGHBIT)==0)
			*q++= *p;
	if(nonascii)
		*nonascii=TRUE;
	return q-s;
}
#ifndef SUN
bootterm(zflag)
{
	if(system("/usr/jerq/bin/32ld", "32ld",
#ifdef DIST
	    zflag? "-z" : "-", "/usr/jerq/mbin/sam.m")){
#else
	    zflag? "-z" : "-", "/n/ikeya/usr/rob/sam/term/a.out")){
#endif
		dprint("sam: can't boot terminal program\n");
		return 0;
	}
	return 1;
}
#endif !SUN
rawmode(raw)
{
	struct sgttyb buf;
	ioctl(0, TIOCGETP, &buf);
	if(raw){
		ioctl(0, TIOCEXCL, (struct sgttyb *)0);
		buf.sg_flags|=RAW|ANYP;
	}else{
		ioctl(0, TIOCNXCL, (struct sgttyb *)0);
#ifndef SUN
		ioctl(0, JTERM, (struct sgttyb *)0);
#endif
		buf.sg_flags&=~(RAW|ANYP);
	}
	ioctl(0, TIOCSETP, &buf);
}
system(s, t, u, v)
	char *s, *t, *u, *v;
{
	int status, pid, l;
	if((pid=fork()) == 0) {
		execl(s, t, u, v, 0);
		_exit(127);
	}
	do; while((l=wait(&status))!=pid && l!=-1);
	if(l==-1)
		status= -1;
	return(status);
}
closeonexec(fd){
	ioctl(fd, FIOCLEX, (struct sgttyb *)0);
}
#ifndef SUN
static int	remotefd;
connectto(machine)
	char *machine;
{
	if(strncmp((uchar *)machine, (uchar *)"inet/", 5)==0)
		remotefd=tcpexec(machine+5, "exec /usr/jerq/bin/sam -R");
	else
		remotefd=tdkexec(machine, "exec /usr/jerq/bin/sam -R");
	if(remotefd<0){
		dprint("sam: can't call %s\n", machine);
		exit(1);
	}
}
join(){
	switch(fork()){
	case 0:
		close(0);
		dup(remotefd);
		execl("/bin/cat", 0);
	default:
		close(1);
		dup(remotefd);
		execl("/bin/cat", 0);
	case -1:
		dprint("sam: can't fork\n");
	}
	exit(1);
}

int
tcpexec(host, cmd)
	char *host;
	char *cmd;
{
	int pfd[2];

	if (pipe(pfd)<0)
		return -1;
	switch(fork()) {
	case -1:
		return -1;
	case 0:
		break;
	default:
		close(pfd[0]);
		return(pfd[1]);
	}
	close(pfd[1]);
	if (dup2(pfd[0], 0)<0 || dup2(pfd[0], 1)<0 || dup2(pfd[0], 2)<0) {
		dprint("sam: can't redirect\n");
		exit(1);
	}
	close(pfd[0]);
	execl("/usr/inet/bin/rsh", host, cmd, 0);
	dprint("sam: can't exec rsh\n");
	exit(1);
	return -1;	/* for cyntax */
}
#endif !SUN
