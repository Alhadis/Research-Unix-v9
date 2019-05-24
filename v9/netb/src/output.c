#include "share.h"

char *cmdname[] = {"0?", "nbput", "nbget", "nbupd", "nbread",
	"nbwrt", "nbnami", "nbstat", "nbioctl", "nbtrnc", "10?", "11?"};
/* build the responses, and translate into client terms */
/* the translation scheme here knows client lengths, which will have to
 * be parameterized for clients that aren't vaxes */

responce(cmd)
{	unsigned char *p = inbuf, *q;
	int n;
	/* the common header first */
	hostlong(client.trannum, p);
	p += 4;
	hostshort(client.errno, p);
	p += 2;
	hostchar(client.namiflags, p);
	p += 1;
	p += 1;	/* skip */

	q = p;
	p += 4;		/* that's the total response len */
	p += 4;		/* skip */
	switch(cmd) {
	default:
		fatal("respond(%d)\n", cmd);
	case NBWRT: case NBTRNC: case NBUPD: case NBPUT:
		hostlong(16, q);
		break;
	case NBREAD:
		break;
	case NBNAMI:
		hostlong(client.tag, p);
		p += 4;
		hostlong(client.ino, p);
		p += 4;
		hostshort(client.dev, p);
		p += 2;
		hostshort(client.mode, p);
		p += 2;
		hostlong(client.used, p);	/* what's with this? */
		p += 4;
		goto common;
	case NBSTAT:
		hostlong(client.ino, p);
		p += 4;
		hostshort(client.dev, p);
		p += 2;
		hostshort(client.mode, p);
		p += 2;
common:
		hostshort(client.nlink, p);
		p += 2;
		hostshort(client.uid, p);
		p += 2;
		hostshort(client.gid, p);
		p += 2;
		p += 2;		/* skip rdev */
		hostlong(client.size, p);
		p += 4;
		hostlong(client.ta, p);
		p += 4;
		hostlong(client.tm, p);
		p += 4;
		hostlong(client.tc, p);
		p += 4;
		break;
	}
	if(!client.resplen)
		client.resplen = p - inbuf;
	hostlong(client.resplen, q);
	debug("responding to %s with %d (%d)\n", cmdname[cmd], client.resplen,
		client.errno);
	n = write(cfd, inbuf, (int)client.resplen);
	if(n == client.resplen)
		return;
	if(n < 0)
		fatal("server(%d): write of len %d errno %d\n", cfd, client.resplen, errno);
	fatal("server: write of len %d only sent %d\n", client.resplen, n);
}

/* convert to client stuff */
#ifdef ns32000
#define vax 1
#endif
#if vax == 1
hostlong(n, p)
unsigned char *p;
long n;
{	union {
		long x;
		unsigned char b[4];
	} u;
	int i;

	switch (clienttype) {
		case 'v':
			*(long *)p = n;
			break;
		case 's':
			u.x = n;
			for(i = 0; i < 4; i++)
				*p++ = u.b[3-i];
			break;
	}
}

hostshort(n, p)
unsigned char *p;
{	union {
		short x;
		unsigned char b[2];
	} u;
	int i;

	switch (clienttype) {
		case 'v':
			*(short *)p = n;
			break;
		case 's':
			u.x = n;
			for(i = 0; i < 2; i++)
				*p++ = u.b[1-i];
			break;
	}
}

hostchar(n, p)
unsigned char *p;
{
	*p = n;
}
#endif
#if cray == 1
hostlong(n, p)	/* truncates */
unsigned char *p;
{	union {
		long x;
		unsigned char b[8];
	} u;
	int i;
	u.x = n;
	switch (clienttype) {
		case 'v':
			for(i = 0; i < 4; i++)
				*p++ = u.b[7-i];
			break;
		case 's':
			for(i = 0; i < 4; i++)
				*p++ = u.b[4+i];
			break;
	}
}
hostshort(n, p)	/* truncates */
unsigned char *p;
{	union {
		int x;
		unsigned char b[8];
	} u;
	int i;
	u.x = n;
	switch (clienttype) {
		case 'v':
			for(i = 0; i < 2; i++)
				*p++ = u.b[7-i];
			break;
		case 's':
			for(i = 0; i < 2; i++)
				*p++ = u.b[6+i];
			break;
	}
}
hostchar(n, p)
unsigned char *p;
{
	*p = n;
}
#endif
#if sun == 1
hostlong(n, p)
unsigned char *p;
long n;
{	union {
		long x;
		unsigned char b[4];
	} u;
	int i;

	switch (clienttype) {
		case 'v':
			u.x = n;
			for(i = 0; i < 4; i++)
				*p++ = u.b[3-i];
			break;
		case 's':
			*(long *)p = n;
			break;
	}
}
hostshort(n, p)
unsigned char *p;
{	union {
		short x;
		unsigned char b[2];
	} u;
	int i;

	switch (clienttype) {
		case 'v':
			u.x = n;
			for(i = 0; i < 2; i++)
				*p++ = u.b[1-i];
			break;
		case 's':
			*(short *)p = n;
			break;
	}
}
hostchar(n, p)
unsigned char *p;
{
	*p = n;
}
#endif
