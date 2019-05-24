/* Network file system B.
 * Setup message contains protocol type, max message size, and dev, at least.
 * Maybe add some permissions stuff.
 * The second message contains login names and uids from the client.
 * The third message contains group names and gids from the client.
 * Each message is acked by a single byte (1, 2, resp 3).
 *
 * All headers are translated from the client .../sys/h/netb.h into a global
 * host structure that contains the union of all the fields.  Each type of
 * response is converted back to the client's format as part of sending the
 * response.  server() is in input.c.
 */

#include "share.h"
int cfd, dbgfd, debugflag;
int dtime;

main(argc, argv)
char **argv;
{	int i, n;
	char buf[32];
	cfd = 0;	/* we get invoked by rexec */
	dbgfd = open("/dev/null", 1);
/*	dbgfd = creat("/tmp/zarf", 0644); /* /tmp/zarf is creamed on reboot */
	n = xread(cfd, buf, 16);
	if(n != 16)
		fatal("client: read on setup %d bytes %d, |%s|\n", n, errno, buf);
	n = buf[0];
	hisdev = buf[1] << 8;	/* that's his dev */
	switch(buf[2]) {
	default:
		fatal("unk protocol %c (0%0)\n", buf[2], buf[2]);
	case 't': case 'd':
		proto = buf[2];
		break;
	}
	debugflag = buf[3];
	switch(buf[4]) {
	default:
		fatal("unk client type %c (0%0)\n", buf[4], buf[4]);
	case 'v': case 's':
		clienttype = buf[4];
		break;
	}
	buf[0] = 1;
	write(cfd, buf, 1);
	for(i = 0; i < FILES; i++)
		files[i].fd = -1;
	error("server %d\n", getpid());
	error("CSOURCE=%s\n", getenv("CSOURCE"));
	server(n * 1024);
}

/* read a fixed size off cfd */
xread(fd, buf, cnt)
unsigned char *buf;
{	unsigned char *p = buf;
	int i, n;
	n = 0;
loop:
	i = read(fd, p, cnt - n);
	if(i <= 0)
		fatal("xread(%d) n %d i %d\n", cnt, n, i);
	
	p += i;
	n += i;
	if(n >= cnt)
		return(cnt);
	goto loop;
}

char msgbuf[1024];
/* VARARGS1 */
error(s, a, b, c, d, e, f)
char *s;
{
	sprintf(msgbuf, s, a, b, c, d, e, f);
	write(dbgfd, msgbuf, strlen(msgbuf));
}

/* VARARGS1 */
debug(s, a, b, c, d, e, f)
char *s;
{
	if(!debugflag)
		return;
	sprintf(msgbuf, s, a, b, c, d, e, f);
	write(dbgfd, msgbuf, strlen(msgbuf));
	/*sync();vain attempt to preserve every precious byte of message */
}
/* VARARGS1 */
fatal(s, a, b, c, d, e, f)
{
	sprintf(msgbuf, s, a, b, c, d, e, f);
	write(dbgfd, msgbuf, strlen(msgbuf));
	exit(1);
}
