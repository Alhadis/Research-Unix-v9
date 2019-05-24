#include "jerq.h"
#include "rcv.h"

#define RCVBUFSIZE	1024
#define	HIBUF		1000
#define	LOWBUF		256

static unsigned char rcvbuffer[RCVBUFSIZE];	/* buffer for rcv input */
struct Jrcvbuf Jrcvbuf = {rcvbuffer, rcvbuffer, rcvbuffer, 0, RCVBUFSIZE, 0 };

rcvchar ()
{
	int i;

	if (!Jrcvbuf.cnt)
		return -1;
	i = *Jrcvbuf.out++;
	if(Jrcvbuf.out == &Jrcvbuf.buf[Jrcvbuf.size])
		Jrcvbuf.out = Jrcvbuf.buf;
	if(--Jrcvbuf.cnt == 0)
		P->state &= ~RCV;
	if (Jrcvbuf.blocked && Jrcvbuf.cnt <= LOWBUF)
		Jrcvbuf.blocked = 0;
	return(i);
}

rcvbfill(buf, cnt)
register unsigned char *buf;
{
	register i;
	unsigned char *ebuf;

	if (Jrcvbuf.cnt == Jrcvbuf.size)
		return;
	ebuf = &Jrcvbuf.buf[Jrcvbuf.size];
	i = min (cnt, Jrcvbuf.size - Jrcvbuf.cnt);
	P->state |= RCV;
	Jrcvbuf.cnt += i;
	if (Jrcvbuf.cnt >= HIBUF)
		Jrcvbuf.blocked = 1;
	while (i--) {
		*Jrcvbuf.in++ = *buf++;
		if (Jrcvbuf.in == ebuf)
			Jrcvbuf.in = Jrcvbuf.buf;
	}
}

#define KBDBUFSIZE	128
static unsigned char kbdbuffer[KBDBUFSIZE];	/* buffer for kbd input */
static struct {
	unsigned char *buf;
	unsigned char *in;
	unsigned char *out;
	int cnt;
	int size;
} kbdbuf = {kbdbuffer, kbdbuffer, kbdbuffer, 0, KBDBUFSIZE};

kbdchar ()
{
	int i;

	if(!kbdbuf.cnt)
		return -1;
	i = *kbdbuf.out++;
	if(kbdbuf.out == &kbdbuf.buf[kbdbuf.size])
		kbdbuf.out = kbdbuf.buf;
	if(--kbdbuf.cnt == 0)
		P->state &= ~KBD;
	return(i);
}

kbdread (cp)
unsigned char *cp;
{
	if(kbdbuf.cnt == kbdbuf.size)
		return;
	*kbdbuf.in++ = *cp;
	kbdbuf.cnt++;
	if(kbdbuf.in == &kbdbuf.buf[kbdbuf.size])
		kbdbuf.in = kbdbuf.buf;
}
