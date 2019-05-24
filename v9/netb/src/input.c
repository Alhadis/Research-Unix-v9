#include "share.h"
#include "signal.h"
/* this is the main server routine.  All the real work is in work.c, but
 * the messages are built here, and converted from the client's types */

struct client client;
unsigned char *inbuf, *nmbuf;
int inlen, hisdev, proto, iamroot;
int clienttype;
extern char *cmdname[];

server(len)
{	int i, n;
	inbuf = (unsigned char *) malloc(inlen = len);
	nmbuf = (unsigned char *) malloc(inlen);
	error("max msg %d bytes\n", len);
	iamroot = getuid() == 0;
	getexcepts();	/* read the exception table */
	getuids();
	getgids();
	doneexcepts();
	/* put the root into files[] */
	addroot();
	signals();
loop:
	switch(proto) {
	default:
		fatal("unk protocol %d |%c|\n", proto, proto);
	case 'd':	/* datakit, so messages */
		n = read(cfd, inbuf, inlen);
		if(n < 16)	/* header is known to be 16 bytes long */
			fatal("server read %d (<16)\n", n);
		if(inbuf[0] != NETB)
			fatal("server wanted version %d, got %d\n", NETB, inbuf[0]);
		break;
	case 't':	/* tcp, wretched byte stream */
		n = tcpread();
		break;
	}
	/* now convert the damned structures */
	debug("server %d bytes for %s (%d)\n", n, cmdname[inbuf[1]], inbuf[1]);
	fromclient(n);
	goto loop;
}

gotsig(n)
{
	error("exiting on signal %d\n", n);
	abort();
	exit(1);
}

signals()
{	int i;
	for(i = 1; i < NSIG; i++)
		signal(i, gotsig);
}

tcpread()
{	unsigned char *p;
	int cnt, n, len;
	p = inbuf;
	/* first read a sendb struct, which is known to be 16 bytes long */
	cnt = 0;
moreheader:
	n = read(cfd, p, inlen);
	if(n < 0)
		fatal("tcpread -1 (%d)\n", errno);
	if(n == 0)
		fatal("tcp first read 0\n");
	p += n;
	cnt += n;
	if(cnt < 16)
	goto moreheader;
	/* the first byte must be NETB */
	if(inbuf[0] != NETB)
		fatal("tcp read version %d, not %d\n", inbuf[0], NETB);
	len = clientlong(inbuf + 8);	/* server knows where len is in header */
	if(cnt > len)
		fatal("tcp read %d (>%d)\n", cnt, len);
more:
	if(cnt >= len)
		return(cnt);
	n = read(cfd, p, len - cnt);
	if(n <= 0)
		fatal("tcp read loop %d after %d (%d)\n", n, cnt, errno);
	cnt += n;
	p += n;
	goto more;
}

struct client nilclient;
/* knows what clients send.  when they're not all vaxes, change this */
fromclient(len)
{	unsigned char *p = inbuf;
	client = nilclient;	/* can you remember who sets what? */
	/* first the struct sendb */
	p++;	/* skip version */
	client.cmd = *p++;
	client.flags = *p++;
	p++;	/* skip */
	client.trannum = clientlong(p);
	p += 4;
	client.len = clientlong(p);
	p += 4;
	client.tag = clientlong(p);
	p += 4;
	if(client.len != len)
		fatal("client sent %d, claimed len was %d\n", len, client.len);
	if(client.tag == 0) {
		error("client sent tag 0 (cmd %d)\n", client.cmd);
		client.errno = ENOENT;
		goto respond;
	}
	/* now per individual requirement */
	switch(client.cmd) {
	default:
		fatal("client send unknown command %d %d\n", client.cmd, inbuf[1]);
	case NBPUT:
		if(len != p - inbuf)
			fatal("client put size, %d (!= %d)\n", len, p - inbuf);
		doput();
		break;
	case NBUPD:
		p += 4;	/* skip */
		client.uid = clientshort(p);
		p += 2;
		client.gid = clientshort(p);
		p += 2;
		client.mode = clientshort(p);
		p += 2;
		client.dev = clientshort(p);
		p += 2;
		p += 4;	/* skip */
		client.ta = clientlong(p);
		p += 4;
		client.tm = clientlong(p);
		p += 4;
		if(len != p - inbuf)
			fatal("client upd size, %d (!= %d)\n", len, p - inbuf);
		doupdate();
		break;
	case NBREAD:
		client.count = clientlong(p);	/* sanity check in doread() */
		p += 4;
		client.offset = clientlong(p);
		p += 4;
		if(len != p - inbuf)
			fatal("client read size, %d (!= %d)\n", len, p - inbuf);
		doread();
		break;
	case NBWRT:
		client.count = clientlong(p);	/* sanity check in dowrite() */
		p += 4;
		client.offset = clientlong(p);
		p += 4;
		if(len != p - inbuf + client.count)
			fatal("client write size, %d (!= %d)\n", len, p - inbuf + client.count);
		dowrite(p);
		break;
	case NBNAMI:
		p += 4;	/* skip */
		client.uid = clientshort(p);
		p += 2;
		client.gid = clientshort(p);
		p += 2;
		client.mode = clientshort(p);
		p += 2;
		client.dev = clientshort(p);
		p += 2;
		client.ino = clientlong(p);
		p += 4;
		donami(p, len - (p - inbuf));
		break;
	case NBSTAT:
		client.ta = clientlong(p);
		p += 4;
		p += 4;	/* skip */
		if(len != p - inbuf)
			fatal("client stat too long, %d (> %d)\n", len, p - inbuf);
		dostat();
		break;
	case NBTRNC:
		if(len != p - inbuf)
			fatal("client trunc too long, %d (> %d)\n", len, p - inbuf);
		dotrunc();
		break;
	}
respond:	
	responce(client.cmd);
}
#ifdef ns32000
#define vax 1
#endif
#if vax == 1
/* clients to vax */
clientlong(p)
unsigned char *p;
{
	switch (clienttype) {
		case 'v':
			return(*(long *)p);
		case 's':
			return(rev4(p));
	}
}

clientshort(p)
unsigned char *p;
{
	switch (clienttype) {
		case 'v':
			return(*(short *)p);
		case 's':
			return(rev2(p));
	}
}
#endif
#if cray == 1
/* client to cray */
clientlong(p)
unsigned char *p;
{	union {
		int x;
		unsigned char b[8];
	} u;
	int i;
	u.x = 0;
	switch (clienttype) {
		case 'v':
			for(i = 0; i < 4; i++)
				u.b[7-i] = *p++;
			return(u.x);
		case 's':
			for(i = 0; i < 4; i++)
				u.b[4+i] = *p++;
			return(u.x);
	}
}
clientshort(p)
unsigned char *p;
{	union {
		int x;
		unsigned char b[8];
	} u;
	int i;
	u.x = 0;
	switch (clienttype) {
		case 'v':
			for(i = 0; i < 2; i++)
				u.b[7-i] = *p++;
			return(u.x);
		case 's':
			for(i = 0; i < 2; i++)
				u.b[6+i] = *p++;
			return(u.x);
	}
}
#endif
#if sun == 1
/* client to sun */
clientlong(p)
unsigned char *p;
{
	switch (clienttype) {
		case 'v':
			return(rev4(p));
		case 's':
			return(*(long *)p);
	}
}

clientshort(p)
unsigned char *p;
{
	switch (clienttype) {
		case 'v':
			return(rev2(p));
		case 's':
			return(*(short *)p);
	}
}
#endif

rev4(p)
unsigned char *p;
{	union {
		long x;
		unsigned char b[4];
	} u;
	int i;
	for(i = 0; i < 4; i++)
		u.b[3-i] = *p++;
	return(u.x);
}
rev2(p)
unsigned char *p;
{	union {	/* this better work! */
		short x;
		unsigned char b[2];
	} u;
	int i;
	u.x = 0;
	for(i = 0; i < 2; i++)
		u.b[1-i] = *p++;
	return(u.x);
}
