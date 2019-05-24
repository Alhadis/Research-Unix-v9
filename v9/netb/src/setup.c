/* the client set up end of the connection */
/* the first message is 16 bytes, (buf-len in 1024's, dev, prot, dbg, 13 unused)
 * the second message is the client's passwd file (name uid '\n') and zeros
 * the third message is client's gid's and zeros
 * each of these is acked by a single byte of 1, 2, resp 3.
 */
#include "share.h"
#include "pwd.h"
#include "grp.h"
struct stat rootstat;
extern int errno;
extern char *sys_errlist[];
int dbgfd = 2, devnum;

/* setup host-name remote-server-to-execute mount-point protocol[dt] */
main(argc, argv)
char **argv;
{	int fd;
	if(argc != 6 && argc != 7)
		fatal("usage: setup host server mount protocol devnum [dbg]\n");
	switch(argv[4][0]) {
	default:	/* cf /n/bowell/usr/jerq/src/sam/io.c */
		fatal("weird net type %s\n", argv[4]);
	case 'd':	/* datakit */
#ifdef OLD
		fd = tdkexec(argv[1], argv[2]);
#else
		fd = ipcopen(ipcpath(argv[1], "dk", "fsb"), "heavy");
#endif
		break;
	case 't':
#ifdef OLD
		fd = tcp_rcmd(argv[1], "shell", "pjw", "pjw", "share/zarf", 0);
#else
		fd = ipcrexec(argv[1], "heavy", argv[2]);
#endif
		break;
	}
	devnum = atoi(argv[5]);
	errno = 0;
	if(fd >= 0)
		setup(fd, argv[3], argv[4], argc == 7? argv[6]: 0);
	else
		perror("couldn't call");
	debug("setup(%s,%s,%s,%s) errno %d\n", argv[1], argv[2], argv[3], argv[4],
		errno);
}

char msg[16];
setup(fd, dir, net, dbg)
char *dir, *net;
{	struct stat stb;
	int i;
	if(stat(dir, &rootstat) < 0)
		fatal("setup: couldn't stat dir %d\n", dir);
	msg[0] = 5;	/* 5*1024 is the largest message i'll send */
		/* that number is known in the kernel in netb.c */
	msg[1] = devnum;	/* client dev, when shifted left 8 */
	msg[2] = net[0];	/* so server knows about messages */
	msg[3] = dbg;
#if mc68000
	msg[4] = 's';
#endif
#if vax
	msg[4] = 'v';
#endif
#if cray
	msg[4] = 'c';
#endif
	/* the next 12 bytes could be used for authentication or something */
	i = write(fd, msg, 16);
	if(i != 16)
		fatal("write on setup returned %d\n", i);
	i = read(fd, msg, 16);
	if(i != 1)
		fatal("read on setup returned %d |%s|\n", i, msg);
	if(msg[0] != 1)
		fatal("setup read char %d\b", msg[0]);
	/* whew, paranoia is costly, and they're still out there */
	/* now send uid table */
	senduid(fd);
	/* now send gid table */
	sendgid(fd);
	i = fmount(4 /*fstyp*/, fd, dir, devnum<<8);
	if(i < 0)
		fatal("gmount returned %d, errno %d:%s\n", i, errno, sys_errlist[errno]);
	/* and that's that */
}

char uidbuf[5*1024];	/* 1024*msg[0] */
senduid(cfd)
{	struct passwd *p;
	char *s = uidbuf, *t;
	int i;
	while(p = getpwent()) {
		for(t = p->pw_name; *t; t++)
			*s++ = *t;
		*s++ = ' ';
		sprintf(s, "%d\n", p->pw_uid);
		while(*s)
			s++;
	}
	endpwent();
	*s++ = 0;
	if(s >= uidbuf + sizeof(uidbuf))
		fatal("client password file too big\n");
	/* send and ack */
	i = write(cfd, uidbuf, sizeof(uidbuf));	/* write has known length */
	if(i != sizeof(uidbuf))
		fatal("write uid %d (%d)\n", i, errno);
	i = read(cfd, uidbuf, 16);
	if(i != 1)
		fatal("senduid ack read %d (%d)\n", i, errno);
	if(uidbuf[0] != 2)
		fatal("send uid ack was %d not 2\n", uidbuf[0]);
}

sendgid(cfd)
{	struct group *p;
	char *s = uidbuf, *t;
	int i;
	for(i = 0; i < sizeof(uidbuf); i++)
		uidbuf[i] = 0;
	while(p = getgrent()) {
		for(t = p->gr_name; *t; t++)
			*s++ = *t;
		*s++ = ' ';
		sprintf(s, "%d\n", p->gr_gid);
		while(*s)
			s++;
	}
	endgrent();
	*s++ = 0;
	if(s >= uidbuf + sizeof(uidbuf))
		fatal("client group file too big\n");
	/* send and ack */
	i = write(cfd, uidbuf, sizeof(uidbuf));
	if(i != sizeof(uidbuf))
		fatal("write gid %d (%d)\n", i, errno);
	i = read(cfd, uidbuf, 16);
	if(i != 1)
		fatal("sendgid ack read %d (%d)\n", i, errno);
	if(uidbuf[0] != 3)
		fatal("send gid ack was %d not 2\n", uidbuf[0]);
}

char msgbuf[1024];
/* VARARGS1 */
fatal(s, a, b, c, d, e, f)
char *s;
{
	sprintf(msgbuf, s, a, b, c, d, e, f);
	write(dbgfd, msgbuf, strlen(msgbuf));
	exit(1);
}

/* VARARGS1 */
debug(s, a, b, c, d, e, f)
char *s;
{
	sprintf(msgbuf, s, a, b, c, d, e, f);
	write(dbgfd, msgbuf, strlen(msgbuf));
}
