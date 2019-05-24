#ifndef	JERQ_H
#define	JERQ_H

#ifdef X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef BSD
#include <sys/time.h>
#endif BSD
#endif X11
#ifdef SUNTOOLS
#include <sunwindow/window_hs.h>
#undef	Rect
#endif SUNTOOLS

#ifndef NULL
#define NULL 0
#endif

#define	nap(x)		jnap(x)
#define	wait(x)		jwait(x)
#define Menu		JMenu
#define sleep(x)	Jsleep(x)
#define alarm(x)	Jalarm(x)
#define own()		(P->state|MOUSE)

typedef int	Word;		/* 32 bits */

typedef unsigned int	UWord;	/* 32 bits */

typedef struct Point {		/* short and to the point	*/
	short	x;
	short	y;
} Point;

typedef struct Rectangle {
	Point origin;
	Point corner;
} Rectangle;
#define cor corner
#define org origin

typedef struct Bitmap {
#ifdef	X11
	Drawable dr;
#endif	X11
#ifdef	SUNTOOLS
	char *dr;
#endif	SUNTOOLS
	Rectangle rect;
	int flag;
#define BI_OFFSCREEN	1		/* Offscreen if set */
} Bitmap;

typedef struct Menu{
	char	**item;			/* string array, ending with 0	*/
	char	*(*generator)();	/* used if item == 0		*/
	short	prevhit;		/* private to menuhit()		*/
	short	prevtop;		/* private to menuhit() 	*/
} Menu;

struct Mouse {
	Point xy;
	int buttons;
	unsigned long time;
};

#define	min(x,y)	(((x) < (y)) ? (x) : (y))
#define	max(x,y)	(((x) > (y)) ? (x) : (y))

#ifdef X11
typedef Pixmap	Texture;	
typedef Pixmap	Texture32;	
#define	Font	XFontStruct	
#define fontheight(fp)	((fp)->max_bounds.ascent + (fp)->max_bounds.descent)
#define fontwidth(fp)	((fp)->max_bounds.width)
#define fontnchars(fp)	((fp)->max_char_or_byte2+1)
#endif X11
#ifdef SUNTOOLS
typedef short	*Texture;
typedef Word	*Texture32;
typedef Pixfont *Font;
#define fontheight(fp)	((*fp)->pf_defaultsize.y)
#define	fontwidth(fp)	((*fp)->pf_defaultsize.x)
#define fontnchars(fp)	255
#define	Cursor	JCursor
typedef struct Cursor {
	short	*bits;
	short	hotx;
	short	hoty;
} Cursor;
#endif SUNTOOLS

struct JProc {
	int	state;
	Cursor	*cursor;
};
#define RESHAPED	1		/* window has been changed */
#define KBD		2		/* we have keyboard input */
#define RCV		4		/* recevied from "host" proc */
#define MOUSE		8		/* we always have the mouse */
#define	SEND		16		/* for request compatability */
#define	CPU		32
#define ALARM		64

typedef int Code;

#if defined(sparc) || defined(lint)
#define Rect(a,b,c,d)	SRect(a,b,c,d)
extern	Point		Pt();
extern	Rectangle	SRect();
extern	Rectangle	Rpt();
#else
#define	Pt(x, y)	 ((((short)(x))<<16)|((unsigned short)(y)))
#define	Rect(x1, y1, x2, y2)	Pt(x1, y1), Pt(x2, y2)
#define	Rpt(p1, p2)		(p1), (p2)
#endif

#define	muldiv(a,b,c)	((long)((a)*((long)b)/(c)))

/*
 * Function Codes
 */
#ifdef	X11
#define	F_STORE	(GXcopy)		/* target = source */
#define	F_OR	(GXor)			/* target |= source */
#define	F_CLR	(GXandInverted)		/* target &= ~source */
#define	F_XOR	(GXxor)			/* target ^= source */
#endif	X11
#ifdef	SUNTOOLS
#define	F_STORE	(PIX_SRC)		/* target = source */
#define	F_OR	(PIX_SRC | PIX_DST)	/* target |= source */
#define	F_CLR	(PIX_NOT(PIX_SRC)&PIX_DST)	/* target &= ~source */
#define	F_XOR	(PIX_SRC ^ PIX_DST)	/* target ^= source */
#endif	SUNTOOLS

#define button(i)		(mouse.buttons&(8>>i))
#define button1()		(mouse.buttons&4)
#define button2()		(mouse.buttons&2)
#define button3()		(mouse.buttons&1)
#define button12()		(mouse.buttons&6)
#define button13()		(mouse.buttons&5)
#define button23()		(mouse.buttons&3)
#define button123()		(mouse.buttons&7)

Rectangle getrectb(), getrect();
#define getrect1()		getrectb(4,1)
#define getrect2()		getrectb(2,1)
#define getrect3()		getrectb(1,1)
#define getrect12()		getrectb(6,1)
#define getrect13()		getrectb(5,1)
#define getrect23()		getrectb(3,1)
#define getrect123()		getrectb(7,1)

extern Point add(), sub(), mul(), div(), string();
extern Rectangle rsubp(), raddp(), inset();
extern Bitmap *balloc();
extern char *gcalloc(), *calloc();
extern void bfree(), gcfree();
extern void rectf(), rectfD(), bitblt(), bitbltD(), texture(), evtomouse();

#define alloc(n) calloc(n,1)

#ifdef X11
extern	Texture		ToTexture();
extern	Texture32	ToTexture32();
extern	GC		gc;
extern	Display		*dpy;
extern	int		fgpix, bgpix;
extern	Colormap	colormap;
extern	XColor		fgcolor, bgcolor;
#define	jerqsync()	XSync(dpy, 0)
#endif X11
#ifdef SUNTOOLS
extern	Pixwin		*displaypw;
extern	int		damagedone;
#define	ToTexture(x)	x
#define	ToTexture32(x)	x
#define	jerqsync()
#endif SUNTOOLS

extern	Font	getfont();
extern	Rectangle Drect;
extern	Bitmap	display, Jfscreen, ToBitmap();
extern	Point	Joffset;
extern	int	displayfd;
extern	int	jerqrcvmask;
extern	Cursor	ToCursor(), *cursswitch(), normalcursor;
extern	struct	Mouse mouse;
extern	struct	JProc *P;
extern	Font	defont;

#endif JERQ_H
