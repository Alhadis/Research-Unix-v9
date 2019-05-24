#include <jerq.h>
#include <stdio.h>

#ifdef	SUNTOOLS
#undef F_STORE
#define	F_STORE	(PIX_DONTCLIP|PIX_SRC)
#endif	SUNTOOLS

#define SHORTSIZE	16
#define SHORT		short

#define sendword(w)	(sendch(w&255),sendch((w>>8)&255))

SHORT buffer[2048/SHORTSIZE], raster[2048/SHORTSIZE];	/* Room to spare */

static int ctype, count; static SHORT *p1, *endraster;
FILE *filep, *popen();

sendbitmap(bp,r,filnam)
Bitmap *bp; Rectangle r; char *filnam;
{
	register i; int nrasters, rastwid; Rectangle rrast;
	int pflag = 0;
	Bitmap *bbuffer;

	if (filnam[0] != '|')
		filep=fopen(filnam, "w");
	else
		filep=popen(filnam+2, "w"), pflag = 1;
	if (!filep) {
		pflag = 0;
		return 1;
	}

	nrasters = r.corner.y-r.origin.y;
	i        = r.corner.x-r.origin.x;
	rastwid  =(i+SHORTSIZE-1)/SHORTSIZE;
	bbuffer = balloc(Rect(0,0,rastwid*SHORTSIZE+32,1));
	endraster= raster+rastwid-1;
	sendword(0);
	sendword(r.origin.x - Jfscreen.rect.origin.x);
	sendword(r.origin.y - Jfscreen.rect.origin.y);
	sendword(r.corner.x - Jfscreen.rect.origin.x);
	sendword(r.corner.y - Jfscreen.rect.origin.y);

	rectf(bbuffer,bbuffer->rect,F_CLR);
	for (i=0; i<rastwid; i++) raster[i] = 0;
	rrast=r;
	rectf(bp,r,F_XOR);

	for (; rrast.origin.y<r.corner.y; rrast.origin.y++) {
		rrast.corner.y = rrast.origin.y+1;
		rectf(bp,rrast,F_XOR);
		bitblt(bp,rrast,bbuffer,Pt(0,0),F_STORE);
		getrast(bbuffer,buffer);
		for (i=0; i<rastwid; i++) raster[i] ^= buffer[i];
		sendrast();
		for (i=0; i<rastwid; i++) raster[i]  = buffer[i];
	}

	bfree(bbuffer);
	if (!pflag)
		fclose(filep);
	else
		pclose(filep);
	return 0;
}

static sendrast()
{
	SHORT *p2;

	p1=p2=raster;
	do {
		if (p1 >= p2) {
			p2=p1+1; count=2;
			ctype=(*p1 == *p2);

		} else if ((*p2 == *(p2+1)) == ctype) {
			if (++count >= 127) {
				sendbits();
				p1=p2+2;
			} else p2++;

		} else if (ctype) {
			sendbits();
			p1=p2+1;
			ctype=0;

		} else {
			count--; sendbits();
			p1=p2;
			ctype=1;
		}
	} while (p2<endraster);

	if (p1 > endraster) return;
	if (p2 > endraster) count--;
	sendbits();
}

static sendbits()
{
	int c;
	c=count; if (ctype) { c += 128; count=1; }
	sendch(c);
	sendnch(sizeof(SHORT)*count,(char *)p1);
}

sendch(c)
int c;
{
	putc(c, filep);
}

sendnch(n,str)
register int n; register char *str;
{
	while (n-- > 0)
		putc(*str++, filep);
}
