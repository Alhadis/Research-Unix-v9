#include "jerq.h"

#ifdef	BSD
#include <fcntl.h>
#else	/* V9 */
#include <sys/filio.h>
#endif BSD
#ifdef	SUNTOOLS
#include <signal.h>
#include <sys/ioctl.h>
#include <suntool/fullscreen.h>
#endif SUNTOOLS

Rectangle	Drect;
Bitmap		display, Jfscreen;
Point		Joffset;
struct Mouse	mouse;
static struct	JProc sP;
struct JProc	*P;
Font		defont;
int		mouse_alive = 0;
int		jerqrcvmask = 1;
int		displayfd;
Cursor		normalcursor;
static int	hintwidth, hintheight, hintflags;
static		Jlocklevel;

#ifdef	X11
GC		gc;
Display		*dpy;
int		fgpix, bgpix;
Colormap	colormap;
XColor		fgcolor, bgcolor;
static unsigned long inputmask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
				 StructureNotifyMask|ExposureMask|KeyPressMask|
				 PointerMotionHintMask;
#endif X11
#ifdef SUNTOOLS
Pixwin		*displaypw;
int		damagedone;
static struct	fullscreen *Jfscp;
static	int	screendepth;
#endif SUNTOOLS

static short arrow_bits[] = {
	0x0000, 0x0008, 0x001c, 0x003e,
	0x007c, 0x00f8, 0x41f0, 0x43e0,
	0x67c0, 0x6f80, 0x7f00, 0x7e00,
	0x7c00, 0x7f00, 0x7fc0, 0x0000
};

static short arrow_mask_bits[] = {
	0x0008, 0x001c, 0x003e, 0x007f,
	0x00fe, 0x41fc, 0xe3f8, 0xe7f0,
	0xffe0, 0xffc0, 0xff80, 0xff00,
	0xff00, 0xffc0, 0xffe0, 0xffc0
};

/* This must be called before initdisplay */
mousemotion ()
{
	mouse_alive = 1;
#ifdef X11
	inputmask |= PointerMotionMask;
#endif X11
}

#ifdef X11
initdisplay(argc, argv)
int argc;
char *argv[];
{
	int i;
	XSetWindowAttributes xswa;
	XSizeHints sizehints;
	XWindowAttributes xwa;
	char *font;
	char *geom = 0;
	int flags;
	int width, height, x, y;
	char **ap;

	if(!(dpy= XOpenDisplay(NULL))){
		perror("Cannot open display\n");
		exit(-1);
	}
	displayfd = dpy->fd;

	if (jerqrcvmask)
#ifdef	BSD
		fcntl(1, F_SETFL, FNDELAY);;
#else
		ioctl(1, FIOWNBLK, 0);
#endif
	font = XGetDefault(dpy, argv[0], "JerqFont");
	if(font == NULL)
		font = "fixed";
	bzero(&sizehints, sizeof(sizehints));
	ap = argv;
	i = argc;
	while(i-- > 0){
		if(!strcmp("-fn", ap[0])){
			font = ap[1];
			i--; ap++;
		}
		else if(ap[0][0] == '='){
			geom = ap[0];	
			flags = XParseGeometry(ap[0],&x,&y,&width,&height);
			if(WidthValue & flags){
				sizehints.flags |= USSize;
				sizehints.width = width;
			}
			if(HeightValue & flags){
	    			sizehints.flags |= USSize;
				sizehints.height = height;
			}
			if(XValue & flags){
				if(XNegative & flags)
				  x=DisplayWidth(dpy,DefaultScreen(dpy))+x 
					- sizehints.width;
				sizehints.flags |= USPosition;
				sizehints.x = x;
			}
			if(YValue & flags){
				if(YNegative & flags)
				  y=DisplayHeight(dpy,DefaultScreen(dpy))+y
					-sizehints.height;
				sizehints.flags |= USPosition;
				sizehints.y = y;
			}
		}
		ap++;
	}
	defont = getfont(font);
	P = &sP;
	sizehints.width_inc = sizehints.height_inc = 1;
	sizehints.min_width = sizehints.min_height = 20;
	sizehints.flags |= PResizeInc|PMinSize;
	if(!geom){
		sizehints.width = defont.max_bounds.width * 80;
		sizehints.height = (defont.max_bounds.ascent +
				 defont.max_bounds.descent) * 24;
		sizehints.flags |= PSize;
		jerqsizehints();
		if (hintwidth)
			sizehints.width = hintwidth;
		if (hintheight)
			sizehints.height = hintheight;
		if (hintflags) {
			sizehints.min_width = hintwidth;
			sizehints.min_height = hintheight;
		}
	}
	xswa.event_mask = 0;
	bgpix = xswa.background_pixel = WhitePixel(dpy, 0);
	fgpix = xswa.border_pixel = BlackPixel(dpy, 0);
	display.dr = XCreateWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		sizehints.x,sizehints.y, sizehints.width, sizehints.height,
		2,0,InputOutput, DefaultVisual(dpy, DefaultScreen(dpy)),
		CWEventMask | CWBackPixel | CWBorderPixel, &xswa);
	XSetStandardProperties(dpy, display.dr, argv[0], argv[0],
				None, argv, argc, &sizehints);
	XMapWindow(dpy, display.dr);
	colormap = XDefaultColormap(dpy, 0);
	fgcolor.pixel = fgpix;
	bgcolor.pixel = bgpix;
	XQueryColor(dpy, colormap, &fgcolor);
	XQueryColor(dpy, colormap, &bgcolor);
	gc = XCreateGC(dpy, display.dr, 0, NULL);
	XSetForeground(dpy, gc, fgpix);
	XSetBackground(dpy, gc, bgpix);
	XSetFont(dpy, gc, defont.fid);
	XSetLineAttributes(dpy, gc, 0, LineSolid, CapNotLast, JoinMiter);
	Drect.origin.x = 0;
	Drect.origin.y = 0;
	Drect.corner.x = sizehints.width;
	Drect.corner.y = sizehints.height;
	display.rect = Drect;
#ifdef XBUG
	Jfscreen = display;
#else
	Jfscreen.dr = RootWindow(dpy, DefaultScreen(dpy));
	Jfscreen.rect.origin.x = 0;
	Jfscreen.rect.origin.y = 0;
	Jfscreen.rect.corner.x = DisplayWidth(dpy, DefaultScreen(dpy));
	Jfscreen.rect.corner.y = DisplayHeight(dpy, DefaultScreen(dpy));
#endif XBUG
	normalcursor = ToCursor(arrow_bits, arrow_mask_bits, 1, 15);
	cursswitch(&normalcursor);
	XSelectInput(dpy, display.dr, inputmask);
	for(;;) {
		if (sizehints.flags & USPosition) {
			XGetWindowAttributes(dpy, display.dr, &xwa);
			if (xwa.map_state != IsUnmapped) {
#ifndef XBUG
				Joffset.x = xwa.x;
				Joffset.y = xwa.y;
#endif XBUG
				break;
			}
		} else if (P->state & RESHAPED)
			break;
		while (XPending(dpy))
			handleinput();
	}
}

Font
getfont(s)
char *s;
{
	static Font f;
	Font *fp;

	fp = XLoadQueryFont(dpy, s);
	return fp ? *fp : f;
}
#endif X11

#ifdef	SUNTOOLS
initdisplay (argc, argv)
int argc;
char **argv;
{
	extern char *getenv();
	extern struct pixrectops mem_ops;
	int gfx;
	int designee;
	struct inputmask mask;
	struct rect gfxrect;
	static winch_catcher();

	fcntl(1, F_SETFL, FNDELAY);
	signal(SIGWINCH, winch_catcher);
	gfx = open(getenv("WINDOW_GFX"), 0);
	if(gfx < 0){
		perror("cannot get graphics window");
		exit(1);
	}
	designee = win_nametonumber(getenv("WINDOW_GFX"));
	displayfd = win_getnewwindow();
	win_insertblanket(displayfd, gfx);
	close(gfx);
	defont = pf_default();
	displaypw = pw_open(displayfd);
	win_getrect(displayfd, &gfxrect);
	screendepth = displaypw->pw_pixrect->pr_depth;
	if(!(displaypw->pw_prretained =
		mem_create(gfxrect.r_width, gfxrect.r_height, 1)))
                	perror("initdisplay: mem_create");
	display.dr = (char *)displaypw;
	Drect.origin.x = Drect.origin.y = 0;
	Drect.corner.x = gfxrect.r_width;
	Drect.corner.y = gfxrect.r_height;
	display.rect = Drect;
	P = &sP;
	input_imnull(&mask);
	if(mouse_alive)
		win_setinputcodebit(&mask, LOC_MOVE);
	win_setinputcodebit(&mask, MS_LEFT);
	win_setinputcodebit(&mask, MS_MIDDLE);
	win_setinputcodebit(&mask, MS_RIGHT);
	win_setinputcodebit(&mask, LOC_DRAG);
	mask.im_flags |= IM_NEGEVENT;
	mask.im_flags |= IM_ASCII;
	win_setinputmask(displayfd, &mask, 0, designee);
	normalcursor = ToCursor(arrow_bits, arrow_mask_bits, 1, 15);
	cursswitch(&normalcursor);
	rectf(&display, Drect, F_CLR);
	Joffset.x = 0;
	Joffset.y = 0;
	P->state |= RESHAPED;
	if (damagedone)
		fixdamage();
}

/*
 *	Catch SIGWINCH signal when window is damaged
 */
static
winch_catcher ()
{
	damagedone = 1;
}

fixdamage()
{
	struct rect gfxrect;
	int x, y;
	
	damagedone = 0;
	pw_damaged(displaypw);
	win_getrect(displayfd, &gfxrect);
	if (Drect.corner.x != gfxrect.r_width ||
	    Drect.corner.y != gfxrect.r_height) {
		Drect.corner.x = gfxrect.r_width;
		Drect.corner.y = gfxrect.r_height;
		display.rect = Drect;
		P->state |= RESHAPED;
		pw_donedamaged(displaypw);
		pr_destroy(displaypw->pw_prretained);
		displaypw->pw_prretained = 0;
		rectf(&display, Drect, F_CLR);
		displaypw->pw_prretained =
		   mem_create(gfxrect.r_width, gfxrect.r_height, 1);
	}
	else {
		pw_repairretained(displaypw);
		pw_donedamaged(displaypw);
	}
}

Font
getfont(s)
char *s;
{
	return(pf_open(s));
}
#endif SUNTOOLS

request(what)
int what;
{
	if (!(what & RCV)) {
		jerqrcvmask = 0;
		close(0);
		close(1);
	}
}

Bitmap *
balloc (r)
Rectangle r;
{
	Bitmap *b;

	b = (Bitmap *)malloc(sizeof (struct Bitmap));
#ifdef X11
	b->dr = XCreatePixmap(dpy, display.dr, r.cor.x-r.org.x,
		r.cor.y-r.org.y, DefaultDepth(dpy, 0));
#endif X11
#ifdef SUNTOOLS
	b->dr = (char *)mem_create(r.cor.x - r.org.x, r.cor.y - r.org.y, 1);
#endif SUNTOOLS
	b->flag = BI_OFFSCREEN;
	b->rect=r;
	return b;
}

void
bfree(b)
Bitmap *b;
{
	if(b){
#ifdef X11
		XFreePixmap(dpy, b->dr);
#endif X11
#ifdef SUNTOOLS
		pr_destroy((Pixrect *)b->dr);
#endif SUNTOOLS
		free((char *)b);
	}
}

Point
string (f, s, b, p, c)
Font *f;
char *s;
Bitmap *b;
Point p;
Code c;
{
	if(b->flag & BI_OFFSCREEN)
		p = sub(p, b->rect.origin);
#ifdef X11
	XSetFunction(dpy, gc, c);
	XDrawString(dpy, b->dr, gc, p.x, p.y + f->max_bounds.ascent, s, strlen(s));
#endif X11
#ifdef SUNTOOLS
	if(b->flag & BI_OFFSCREEN){
		struct pr_prpos where;
		where.pr = (Pixrect *)b->dr;
		where.pos.x = p.x;
		where.pos.y = p.y - (*f)->pf_char['A'].pc_home.y;
		pf_text(where, c, *f, s);
	}
	else
		pw_text((Pixwin *)b->dr, p.x,
			p.y - (*f)->pf_char['A'].pc_home.y,
			c, *f, s);
#endif SUNTOOLS
	return(add(p, Pt(strwidth(f,s),0)));
}

int
strwidth (f, s)
Font *f;
register char *s;
{
#ifdef X11
	return XTextWidth(f, s, strlen(s));
#endif X11
#ifdef SUNTOOLS
	struct pr_size size;
	size = pf_textwidth(strlen(s), *f, s);
	return(size.x);
#endif SUNTOOLS
}

#ifdef	X11
/*
 * Convert a blit style texture to a pixmap which can be used in tiling
 * or cursor operations.
 */
Texture
ToTexture(bits)
short bits[];
{
	static XImage *im;
	Pixmap pm;

	if (!im)
		im = XCreateImage(dpy, XDefaultVisual(dpy, 0), 1,
				  XYBitmap, 0, (char *)bits, 16, 16, 8, 2);
	else
		im->data = (char *)bits;
	XSetForeground(dpy, gc, fgpix);
	XSetBackground(dpy, gc, bgpix);
	XSetFunction(dpy, gc, GXcopy);
	pm = XCreatePixmap(dpy, display.dr, 16, 16, 1);
	XPutImage(dpy, pm, gc, im, 0, 0, 0, 0, 16, 16);
	return pm;
}

Cursor
ToCursor (source, mask, hotx, hoty)
short source[], mask[];
{
	Texture sp, mp;
	Cursor c;

	sp = ToTexture(source);
	mp = ToTexture(mask);
	c = XCreatePixmapCursor(dpy, sp,mp, &fgcolor,&bgcolor, hotx,hoty);
	XFreePixmap(dpy, sp);
	XFreePixmap(dpy, mp);
	return(c);
}
#endif X11

#ifdef SUNTOOLS
Cursor
ToCursor (source, mask, hotx, hoty)
short source[], mask[];
{
	Cursor c;

	c.bits = source;
	c.hotx = hotx;
	c.hoty = hoty;
	return(c);
}
#endif SUNTOOLS

char *
gcalloc (nbytes, where)
unsigned long nbytes;
char **where;
{
	*where=(char *)alloc(nbytes);
	return *where;
}

void
gcfree (s)
char *s;
{
	free(s);
}

#ifdef	X11
#undef button
handleinput ()
{
	XEvent ev;
	KeySym key;
	unsigned char s[16], *cp;
	int n;
	Window rw, cw;
	int xr, yr, xw, yw;
	unsigned bstate;

	for(;;){
		XNextEvent(dpy, &ev);
		switch (ev.type) {
		case ButtonPress:
			mouse.buttons |= (8 >> ev.xbutton.button);
			mouse.xy.x = ev.xbutton.x;
			mouse.xy.y = ev.xbutton.y;
			mouse.time = ev.xbutton.time;
			break;
		case ButtonRelease:
			mouse.buttons &= ~(8 >> ev.xbutton.button);
			mouse.xy.x = ev.xbutton.x;
			mouse.xy.y = ev.xbutton.y;
			mouse.time = ev.xbutton.time;
			break;
		case MotionNotify:
			XQueryPointer(dpy, display.dr,
				&rw, &cw, &xr, &yr, &xw, &yw, &bstate);
			if(button123() && bstate==0)
				continue;
			mouse.xy.x = xw;
			mouse.xy.y = yw;
			break;
		case MapNotify:
		case NoExpose:
			break;
		case ConfigureNotify:
#ifndef XBUG
			Joffset.x = ev.xconfigure.x;
			Joffset.y = ev.xconfigure.y;
#endif XBUG
			if (display.rect.corner.x != ev.xconfigure.width ||
			    display.rect.corner.y != ev.xconfigure.height) {
				display.rect.corner.x = ev.xconfigure.width;
				display.rect.corner.y = ev.xconfigure.height;
				Drect = display.rect;
#ifdef XBUG
				Jfscreen = display;
#endif
			}
			break;
		case Expose:
			if (ev.xexpose.count == 0) {
				rectf(&display, Drect, F_CLR);
				P->state |= RESHAPED;
			}
			break;
		case KeyPress:
			mouse.xy.x = ev.xkey.x;
			mouse.xy.y = ev.xkey.y;
			mouse.time = ev.xkey.time;
			n = XLookupString(&ev, s, sizeof(s), NULL, NULL);
			if(n > 0){
				cp = s;
				P->state |= KBD;
				do{
					kbdread(cp);
				} while (--n);
			}
			break;
		default:
			break;
		}
		return;
	}
}
#endif X11

#ifdef SUNTOOLS
#define BUTTON1	0x4
#define BUTTON2	0x2
#define BUTTON3	0x1
handleinput ()
{
        struct inputevent ie;
	long readbytes;
	static grabbed = 0;
	unsigned char c;

	for(;;){
		if(input_readevent(displayfd, &ie) == -1){
			perror("input_readevent: ");
			break;
		}
		mouse.xy.x = ie.ie_locx;
		mouse.xy.y = ie.ie_locy;
		mouse.time = ie.ie_time.tv_sec*1000 + ie.ie_time.tv_usec/1000;
		if(event_is_ascii(&ie)){
			c = ie.ie_code;
			kbdread(&c);
			P->state |= KBD;
		}
		else if(event_is_button(&ie)){
			switch(ie.ie_code){
			case MS_LEFT:
				if(win_inputnegevent(&ie)){
					mouse.buttons &= ~BUTTON1;
				}
				else{
					mouse.buttons |= BUTTON1;
					if(!grabbed){
						win_grabio(displayfd);
						grabbed = 1;
					}
				}
				break;
			case MS_MIDDLE:
				if(win_inputnegevent(&ie)){
					mouse.buttons &= ~BUTTON2;
				}
				else{
					mouse.buttons |= BUTTON2;
					if(!grabbed){
						win_grabio(displayfd);
						grabbed = 1;
					}
				}
				break;
			case MS_RIGHT:
				if(win_inputnegevent(&ie)){
					mouse.buttons &= ~BUTTON3;
				}
				else{
					mouse.buttons |= BUTTON3;
					if(!grabbed){
						win_grabio(displayfd);
						grabbed = 1;
					}
				}
				break;
			}
			if(!button123() && grabbed){
				win_releaseio(displayfd);
				grabbed = 0;
			}
		}
				/* break if there is nothing to read */
		if(ioctl(displayfd, FIONREAD, &readbytes) == -1){
			perror("handleinput:");
			break;
		}
		if(readbytes == 0)
			break;	
	}
}
#endif SUNTOOLS

Jscreengrab()
{
	if (!Jlocklevel) {
#ifdef X11
		while (XGrabPointer(dpy, display.dr, False,
                        inputmask, GrabModeAsync, GrabModeAsync,
			None, *P->cursor, CurrentTime) != GrabSuccess)
				sleep(6);
		XSetSubwindowMode(dpy, gc, IncludeInferiors);
#endif X11
#ifdef SUNTOOLS
		if (screendepth == 1) {
			Jfscp = fullscreen_init(displayfd);
			Jfscreen.dr = (char *)Jfscp->fs_pixwin;
			Jfscreen.rect.origin.x = Jfscp->fs_screenrect.r_left;
			Jfscreen.rect.origin.y = Jfscp->fs_screenrect.r_top;
			Jfscreen.rect.corner.x = Jfscp->fs_screenrect.r_left +
		 	 Jfscp->fs_screenrect.r_width;
			Jfscreen.rect.corner.y = Jfscp->fs_screenrect.r_top +
		  	 Jfscp->fs_screenrect.r_height;
		} else
			Jfscreen = display;
#endif SUNTOOLS
	}
	Jlocklevel++;
}

Jscreenrelease()
{
	if (--Jlocklevel <= 0) {
		Jlocklevel = 0;
#ifdef X11
		XUngrabPointer(dpy, CurrentTime);
		XSetSubwindowMode(dpy, gc, ClipByChildren);
#endif X11
#ifdef SUNTOOLS
		if (screendepth == 1)
			fullscreen_destroy(Jfscp);
#endif SUNTOOLS
	}
}

/* Compatability functions */

ringbell ()
{}

cursinhibit ()
{
}

cursallow ()
{
}

/* misc functions	*/
border (b,r,i,f)
Bitmap *b;
Rectangle r;
int i;
Code f;
{
	rectf(b, Rect(r.origin.x, r.origin.y, r.corner.x, r.origin.y+i), f);
	rectf(b, Rect(r.origin.x, r.corner.y-i, r.corner.x, r.corner.y), f);
	rectf(b, Rect(r.origin.x, r.origin.y+i, r.origin.x+i, r.corner.y-i), f);
	rectf(b, Rect(r.corner.x-i, r.origin.y+i, r.corner.x, r.corner.y-i), f);
}

setsizehints (width, height, flags)
{
	hintwidth = width;
	hintheight = height;
	hintflags = flags;
}
