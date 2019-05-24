#include <jerq.h>

#define SHORTSIZE	16

#define outword(w)	((*ouch)(w) < 0 ? -1 : (*ouch)((w)>>8))
#define outpoint(p)	(outword((p).x) < 0 ? -1 : outword((p).y))
#define outrect(r)	(outpoint((r).origin) < 0 ? -1 : outpoint((r).corner))

static Word buffer[XMAX/WORDSIZE];

static Bitmap bbuffer = { buffer, XMAX/WORDSIZE, 0, 0, XMAX, 1, };

static int ctype, count, rastwid, (*ouch)(); static short *p1;

outbitmap(bp, r, oucharg)
Bitmap *bp; Rectangle r; int (*oucharg)();
{
	Rectangle rrast;

	ouch = oucharg;

	rastwid = (r.corner.x-r.origin.x+SHORTSIZE-1)/SHORTSIZE;
	if (outword(0) < 0 || outrect(r) < 0)
		return -1;

	rectf(&bbuffer, bbuffer.rect, F_CLR);
	rrast=r;
	rectf(bp, r, F_XOR);

	while (rrast.origin.y < r.corner.y) {
		rrast.corner.y = rrast.origin.y+1;
		rectf(bp, rrast, F_XOR);
		bitblt(bp, rrast, &bbuffer, Pt(0,0), F_XOR);
		if (outrast((short *)buffer) < 0) {
			rrast.origin.y++;
			rectf(bp, Rpt(rrast.origin, r.corner), F_XOR);
			return -1;
		}
		bitblt(bp, rrast, &bbuffer, Pt(0,0), F_STORE);
		rrast.origin.y++;
	}
	return 0;
}

static outrast(p2)
register short *p2;
{
	short *endraster;
	p1 = p2;
	endraster = p2+rastwid-1;
	do {
		if (p1 >= p2) {
			p2=p1+1; count=2;
			ctype=(*p1 == *p2);

		} else if ((*p2 == *(p2+1)) == ctype) {
			if (++count >= 127) {
				if (outbits() < 0)
					return -1;
				p1=p2+2;
			} else p2++;

		} else if (ctype) {
			if (outbits() < 0)
				return -1;
			p1=p2+1;
			ctype=0;

		} else {
			count--;
			if (outbits() < 0)
				return -1;
			p1=p2;
			ctype=1;
		}
	} while (p2 < endraster);

	if (p1 > endraster)
		return 0;
	if (p2 > endraster)
		count--;
	if (outbits() < 0)
		return -1;
	return 0;
}

static outbits()
{
	register c; register char *pout;
	c = count;
	if (ctype)
		c += 128, count=1;
	if ((*ouch)(c) < 0)
		return -1;
	c = sizeof(short)*count;
	pout = (char *)p1;
	while (--c >= 0)
		if ((*ouch)(*pout++) < 0)
			return -1;
	return 0;
}
