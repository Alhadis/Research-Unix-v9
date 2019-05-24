#include <sys/types.h>
#ifdef BSD
#include <sgtty.h>
#include <setjmp.h>
#include <errno.h>
#else
#include <sys/stream.h>
#include <sys/ioctl.h>

extern int tty_ld, mesg_ld;
static struct ttydevb	devmodes;
#endif BSD
#include <signal.h>
#include "jerq.h"

static char		umesgf[]=	"/dev/pt/pt109";
static char		*shell;
static struct sgttyb	sttymodes;
static struct tchars	tcharmodes;
int	childid;

extern char	*getenv();

dsegment(b, p, q, f, linetype)
Bitmap *b;
	Point p, q;
	char *linetype;
{
#ifdef X11
	if (linetype) {
		XSetLineAttributes(dpy,gc,0,LineDoubleDash,CapButt,JoinMiter);
		XSetDashes(dpy,gc,0, linetype, strlen(linetype));
	}
#endif X11
	segment(b,p,q,f);
#ifdef X11
	if (linetype)
		XSetLineAttributes(dpy,gc,0,LineSolid,CapButt,JoinMiter);
#endif X11
}

#ifdef BSD
startshell()
{
	if((shell=getenv("SHELL")) == 0)
		shell="sh";
	if (doshell() < 0)
		exit(1);
}
#else
startshell()
{
	if((shell=getenv("SHELL")) == 0)
		shell="sh";
	if (ioctl(3, TIOCGETP, &sttymodes) == -1)
		readmodes();
	else {
		ioctl(3, TIOCGDEV, &devmodes);
		ioctl(3, TIOCGETC, &tcharmodes);
	}
	if (doshell() < 0)
		exit(1);
}

readmodes()
{
	char filename[256], *p;
	int fd;

	if((p=getenv("HOME")) == 0)
		exit(1);
	strcpy(filename, p);
	strcat(filename, "/.sttymodes");
	if ((fd = open(filename, 0)) < 0)
		exit(1);
	read(fd, &sttymodes, sizeof(struct sgttyb));
	read(fd, &devmodes, sizeof(struct ttydevb));
	read(fd, &tcharmodes, sizeof(struct tchars));
	close(fd);
}
#endif BSD

#ifndef BSD
/*
 *	Unpack a message
 */
rcvfill()
{
	register struct mesg *mp;
	register int size;
	char buf[1024];
	int i, n;
	register char *bp;

	n = read(0, buf, sizeof(buf));
	bp = buf;
	mp = (struct mesg *)bp;
	size = mp->losize + (mp->hisize<<8);
	if(n<=0)
		mp->type=M_HANGUP;
	switch (mp->type) {
	case M_HANGUP:
		exit(1);
	case M_DELAY:
	default:
		return;
	case M_DELIM:
	case M_DATA:
		if(size==0)
			return;
		break;
	case M_IOCTL:
		mp->type = M_IOCACK;
		switch (*(int *)(bp+MSGHLEN)) {
		case TIOCSETP:
		case TIOCSETN:
			sttymodes = *(struct sgttyb *)(bp+MSGHLEN+sizeof(int));					size = 0;
			break;
		case TIOCGETP:
			*(struct sgttyb *)(bp+MSGHLEN+sizeof(int)) = sttymodes;
			size=sizeof(struct sgttyb)+sizeof(int);
			break;
		case TIOCSETC:
			tcharmodes = *(struct tchars *)(bp+MSGHLEN+sizeof(int));
			size=0;
			break;
		case TIOCGETC:
			*(struct tchars *)(bp+MSGHLEN+sizeof(int)) = tcharmodes;
			size=sizeof (struct tchars) + sizeof (int);
			break;
		case TIOCSDEV:
			size=0;
			break;
		case TIOCGDEV:
			size=sizeof(struct ttydevb)+sizeof(int);
			*(struct ttydevb *)(bp+MSGHLEN+sizeof(int))=devmodes;
			break;
		default:
			mp->type = M_IOCNAK;
			size = 0;
		}
		mp->magic = MSGMAGIC;		/* safety net */
		mp->losize = size;
		mp->hisize = size>>8;
		write(1, bp, MSGHLEN+size);
		return;
	}
	bp += MSGHLEN;
	rcvbfill(bp, size);
}

sendchar(c)
{
	unsigned char uc = c;

	wrmesgb(&uc, 1);
	delim();
}

sendnchars(n, cp)
char *cp;
{
	wrmesgb(cp, n);
}

wrmesgb(cp, n)
	register char *cp;
	int n;
{
	char wrbuf[128+MSGHLEN];
	register char *bp;
	register struct mesg *mp;
	register int i;

	mp = (struct mesg *)wrbuf;
	mp->type=M_DATA;
	mp->magic=MSGMAGIC;
	mp->losize=n;
	mp->hisize=n>>8;
	if (n <= 128) {
		bp=wrbuf+MSGHLEN;
		i=n;
		while(i--)
			*bp++= *cp++;
		write(1, wrbuf, MSGHLEN+n);
	} else {
		write(1, wrbuf, MSGHLEN);
		write(1, cp, n);
	}
}

delim()
{
	struct mesg delbuf;

	delbuf.type=M_DELIM;
	delbuf.magic=MSGMAGIC;
	delbuf.losize=0;
	delbuf.hisize=0;
	write(1, (char *)&delbuf, MSGHLEN);
}

doshell()
{
	register fd, slave;
	if((fd=ptopen(umesgf))<0){
		return -1;
	}
	if((slave=open(umesgf, 2))==-1){
		close(fd);
		return -1;
	}
	if(ioctl(fd, FIOPUSHLD, &mesg_ld) == -1){
		close(slave);
		close(fd);
		return -1;
	}
	if(ioctl(slave, FIOPUSHLD, &tty_ld) == -1){
		close(slave);
		close(fd);
		return -1;
	}

	switch(childid = fork()){
	case 0:
		/* close every file descriptor in sight, and then some */
		for(fd=0; fd<10; fd++)
			if(fd!=slave)
				close(fd);
		dup(slave); dup(slave); dup(slave); dup(slave);
		close(slave);
		ioctl (0, TIOCSETP, (char *)&sttymodes);
		ioctl (0, TIOCSETC, (char *)&tcharmodes);
		ioctl(0, TIOCSPGRP, 0);
		signal(SIGPIPE, (int (*)())0);
		execlp(shell, shell, 0);
		perror(shell);
		exit(1);
		break;
	case -1:
		close(fd);
		return -1;
	}
	close(slave);
	close(0); close(1);
	dup(fd); dup(fd);
	return(fd);
}
#endif BSD

#ifdef BSD
static jmp_buf env;
hungtty()
{
	longjmp(env, 1);
}

doshell()
{
	static struct  ltchars ltc;
	int discipline;
	long lmode;
	int tty, pgrp, slave;
	register fd, i;

	for(i = 0; i < NSIG; i++)
		signal(i, SIG_DFL);

	tty = open ("/dev/tty", 2);
	ioctl(tty, TIOCGETP, (char *)&sttymodes);
	ioctl(tty, TIOCGETC, (char *)&tcharmodes);
	ioctl(tty, TIOCGETD, (char *)&discipline);
	ioctl(tty, TIOCGLTC, (char *)&ltc);
	ioctl(tty, TIOCLGET, (char *)&lmode);
	ioctl(tty, TIOCNOTTY, (char *)0);
	close (tty);
	if((fd=ptopen(&slave))<0)
		return -1;
	switch(childid = fork()){
	case 0:
		childid = getpid();
		ioctl(slave, TIOCSPGRP, &childid);
		setpgrp (childid, childid);
		dup2(slave, 0); dup2(slave, 1); dup2(slave, 2);
		/* close every file descriptor in sight, and then some */
		for(fd=3; fd<10; fd++)
			close(fd);
		sttymodes.sg_flags &= ~(ALLDELAY | XTABS | CBREAK | RAW);
		sttymodes.sg_flags |= ECHO | CRMOD;
		sttymodes.sg_ispeed = B9600;
		sttymodes.sg_ospeed = B9600;
		tcharmodes.t_brkc = -1;
		ioctl (0, TIOCSETD, (char *)&discipline);
		ioctl (0, TIOCSETP, (char *)&sttymodes);
		ioctl (0, TIOCSETC, (char *)&tcharmodes);
		ioctl (0, TIOCSLTC, (char *)&ltc);
		ioctl (0, TIOCLSET, (char *)&lmode);
		execlp(shell, shell, 0);
		execlp(shell, shell, 0);
		perror(shell);
		exit(1);
		break;
	case -1:
		close(fd);
		return -1;
	}
	close(slave);
	dup2(fd, 0);
	dup2(fd, 1);
	return(fd);
}

ptopen (tty)
int *tty;
{
	int devindex, letter = 0;
	static char ttydev[] = "/dev/ttyxx";
	static char ptydev[] = "/dev/ptyxx";
	int masterfd;

	while (letter < 11) {
	    ttydev [8] = ptydev [8] = "pqrstuvwxyz" [letter++];
	    devindex = 0;

	    while (devindex < 16) {
		ttydev [9] = ptydev [9] = "0123456789abcdef" [devindex++];
		if ((masterfd = open (ptydev, 2)) < 0)
			continue;
		if ((*tty = open (ttydev, 2)) < 0) {
			close(masterfd);
			continue;
		}
		return masterfd;
	    }
	}
	return -1;
}
#endif BSD
