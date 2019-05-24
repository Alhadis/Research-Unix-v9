#include <jerq.h>

Bitmap
ToBitmap(bits, bytewidth, ox, oy, cx, cy)
char *bits;
{
	Bitmap *bm;
	int dx, dy;
#ifdef X11
	XImage *im;
#endif X11
#ifdef SUNTOOLS
	int i, lbytes;
	char *to;
#endif SUNTOOLS

	dx = cx - ox;
	dy = cy - oy;
	bm = balloc(Rect(ox, oy, cx, cy));
#ifdef X11
	im = XCreateImage(dpy, XDefaultVisual(dpy, 0), 1,
				  XYBitmap, 0, bits, dx, dy, 8, bytewidth);
	XSetForeground(dpy, gc, fgpix);
	XSetBackground(dpy, gc, bgpix);
	XSetFunction(dpy, gc, GXcopy);
	XPutImage(dpy, bm->dr, gc, im, 0, 0, 0, 0, dx, dy);
	im->data = (char *)0;
	XDestroyImage(im);
#endif X11
#ifdef SUNTOOLS
	lbytes = mpr_d((Pixrect *)bm->dr)->md_linebytes;
	to = (char *)mpr_d((Pixrect *)bm->dr)->md_image;
	for(i = 0; i < dy; i++) {
		bcopy(bits, to,lbytes);
		to += lbytes;
		bits += bytewidth;
	}
#endif SUNTOOLS
	return *bm;
}
