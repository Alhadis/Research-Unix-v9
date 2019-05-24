#include <jerq.h>

#define	UP	0
#define	DOWN	1
#define NCHFIL	40

#define drstore(r)	rectf(&display, r, F_STORE)
#define drflip(r)	rectf(&display, r, F_XOR)
#define drclr(r)	rectf(&display, r, F_CLR)
#define drstring(s,p)	string(&defont, s, &display, p, F_XOR)

extern Cursor menu3, deadmouse;

char filnam[NCHFIL]="BLITBLT", ppgm[NCHFIL]="| bcan";

char *top_menu[]={
	"choose window",
	"window interior",
	"reverse video",
	"sweep rectangle",
	"whole screen (!)",
	"write file",
	ppgm,
	"exit",
	NULL
};
Menu topmenu={ top_menu };

Rectangle kbdrect(); Point kbdp; int fail;

Bitmap *bp;
Rectangle rect, findproc();

main(argc, argv)
char **argv;
{
	register m;
	int i;
	Rectangle r;

	for(i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'p' && ++i < argc) {
			strncpy(ppgm+2, argv[i], 30);
			ppgm[30+2] = 0;
		}
	}
	request(KBD|MOUSE);
	initdisplay(argc, argv);
	initobjs();
	cursswitch(&menu3);
	checkshape(1);

	for (;; wait(MOUSE)) {
		if (lbuttons(DOWN) != 3) continue;

		cursswitch(NULL);
		m = menuhit(&topmenu,3);
		switch (m) {
		case 0:
		case 1:
			r = findproc(m);
			flash(r);
			break;
		case 2:
			if (bp) {
				Jscreengrab();
				rectf(bp, rect, F_XOR);
				Jscreenrelease();
			}
			break;
		case 3:
			flash(getrect3());
			break;
		case 4:
			flash(Jfscreen.rect);
			break;
		case 5:
			getfilnam();
			sendrect(filnam);
			break;
		case 6:
			sendrect(ppgm);
			break;
		case 7:
			if (!lexit3()) break;
			exit(0);
		}
		cursswitch(&menu3);
	}
}

sendrect(fp)
char *fp;
{
	fail = -1;
	cursswitch(&deadmouse);
	drstore(kbdrect());
	visible(0);
	if (bp) {
		Jscreengrab();
		fail = sendbitmap(bp, rect, fp);
		Jscreenrelease();
	}
	visible(1);
	drstore(kbdrect());
	switch (fail) {
	case -1:
		drstring("No selection", kbdp); break;
	case 0:
		drstring(fp, drstring("Wrote ", kbdp)); break;
	default:
		drstring("Write failed", kbdp); break;
	}
}

getfilnam()
{
	Point p; char str[NCHFIL];
	drstore(kbdrect());
	p=drstring("File (",kbdp); p=drstring(filnam,p); p=drstring("): ",p);

	if (kbdstring(str,NCHFIL,p) > 0) strcpy(filnam,str);
}

#ifdef SUNTOOLS
/*
 * Determine the window mpos is in
 * and return its rectangle.  If flag is
 * set don't return the border.
 */
Rectangle windowrect(mpos, flag)
Point mpos;
int flag;
{
	int wfd, next;
	Rectangle r;
	char name[WIN_NAMESIZE];
	struct rect srect;
	char *getenv();
	Point porigin;

	porigin = Jfscreen.rect.origin;
	if ((wfd = open(getenv("WINDOW_PARENT"), 0)) < 0)
		goto out;
again:
	next = win_getlink(wfd, WL_TOPCHILD);
	close(wfd);
	while (next != WIN_NULLLINK) {
		win_numbertoname(next, name);
		if ((wfd = open(name, 0)) < 0)
			goto out;
		win_getrect(wfd, &srect);
		r.origin.x = srect.r_left;
		r.origin.y = srect.r_top;
		r.corner.x = r.origin.x + srect.r_width;
		r.corner.y = r.origin.y + srect.r_height;
		r = raddp(r, porigin);
		if (ptinrect(mpos, r)) {
			if (flag) {
				porigin = r.origin;
				flag = 0;
				goto again;
			}
			close(wfd);
			return r;
		}
		next = win_getlink(wfd, WL_COVERED);
		close(wfd);
	}
out:
	r.origin.x = r.origin.y = 0;
	r.corner.x = r.corner.y = 0;
	return r;
}

getrast(bm,to)
Bitmap *bm;
register unsigned short *to;
{
	register unsigned short *from;
	register nshorts;
	int remainder;

	from = (unsigned short *)(
		((struct mpr_data *)(
		  ((Pixrect *)bm->dr)->pr_data))->md_image);
	nshorts = (bm->rect.corner.x - bm->rect.origin.x + 15) / 16;
	remainder = (bm->rect.corner.x - bm->rect.origin.x) % 16;
	while(nshorts--)
		*to++ = *from++;
	if (remainder) {
		to--;
		*to &= (0xffff << (16 - remainder));
	}
}
#endif SUNTOOLS

#ifdef X11
/*
 * Determine the window mpos is in
 * and return its rectangle.  If flag is
 * set don't return the border.
 */
Rectangle windowrect(mpos, flag)
Point mpos;
int flag;
{
	Rectangle r;
	int nchildren = 0;
	Window rt, parent, *children;
	int x, y;
	unsigned w, h, bw, d;

	XQueryTree(dpy, RootWindow(dpy, DefaultScreen(dpy)), &rt, &parent,
		   &children, &nchildren);
	while (nchildren-- >= 0) {
		XGetGeometry(dpy, children[nchildren], &rt,
			     &x, &y, &w, &h, &bw, &d);
		r.origin.x = x + bw;
		r.origin.y = y + bw;
		r.corner.x = r.origin.x + w;
		r.corner.y = r.origin.y + h;
		if (ptinrect(mpos, r)) {
			if (!flag)
				r = inset(r, -bw);
			break;
		}
	}
	if (nchildren < 0) {
		r.origin.x = r.origin.y = 0;
		r.corner.x = r.corner.y = 0;
	}
out:
	XFree(children);
	return r;
}

getrast(bm,to)
Bitmap *bm;
register unsigned short *to;
{
	register unsigned short *from;
	register nshorts;
	int remainder;
	XImage *im;

	im = XGetImage(dpy, bm->dr, 0, 0, bm->rect.corner.x - bm->rect.origin.x,
		       1, 1, XYPixmap);
	from = (unsigned short *)im->data;
	nshorts = (bm->rect.corner.x - bm->rect.origin.x + 15) / 16;
	remainder = (bm->rect.corner.x - bm->rect.origin.x) % 16;
	while(nshorts--)
		*to++ = *from++;
	if (remainder) {
		to--;
		*to &= (0xffff << (16 - remainder));
	}
	XDestroyImage(im);
}
#endif X11
