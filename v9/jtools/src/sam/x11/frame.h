#ifndef	FRAME_H
#define	FRAME_H

#if defined(SUNTOOLS) || defined(X11)
#include "jerq.h"
#undef max
#endif
#ifdef JERQ
#include <jerq.h>
#include <font.h>
#include "defont.h"
#endif JERQ
#undef	frinit
#undef	frsetrects
#ifndef BSD
typedef	unsigned short	ushort;
#endif BSD
typedef	unsigned char	uchar;
typedef ushort		Posn;
typedef struct Box{
	short		wid;		/* in pixels */
	short		len;		/* <0 ==> negate and treat as break char */
	union{
		uchar	*BUptr;
		struct{
			short	BUSbc;
			short	BUSminwid;
		}BUS;
	}BU;
}Box;
#define	ptr	BU.BUptr
#define	bc	BU.BUS.BUSbc
#define	minwid	BU.BUS.BUSminwid
typedef struct Frame{
	Font		*font;
	Bitmap		*b;
	Rectangle	r;
	Rectangle	entire;
	Box		*box;
	Posn		p0, p1;
	ushort		left;
	ushort		nbox, nalloc;
	ushort		maxtab;
	ushort		maxcharwid;
	ushort		nchars;
	ushort		nlines;
	ushort		maxlines;
	ushort		lastlinefull;
}Frame;
#define	D	(&display)
#define	B	(f->b)

#undef	charofpt();
#undef	ptofchar();
Posn	charofpt();
Point	ptofchar();
Point	ptofcharptb();
Point	ptofcharnb();
uchar	*allocstr();
uchar	*dupstr();
Point	draw();

#define	LEN(b)	((b)->len<0? 1 : (b)->len)
#if defined(SUNTOOLS) || defined(X11)
#define cwidth(c,ft) fontwidth(ft)
#define fheight(ft) fontheight(ft)
#define fnchars(ft) fontnchars(ft)
#endif
#ifdef JERQ
#define cwidth(c,ft) (ft->info[c].width)
#define fheight(ft) (ft->height)
#define fnchars(ft) (ft->n)
#endif JERQ
#endif FRAME_H
