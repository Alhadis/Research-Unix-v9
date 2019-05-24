#include "jerq.h"

/*
 * Read back the value of a pixel
 */
getpoint(b,p)
Bitmap *b;
Point p;
{
	int bit;
#ifdef X11
	XImage *im;

	if(b->flag & BI_OFFSCREEN)
		p = sub(p, b->rect.origin);
	im = XGetImage(dpy, b->dr, p.x,  p.y, 1, 1, 1, XYPixmap);
	bit = (*im->data != 0);
	XDestroyImage(im);
#endif X11
#ifdef SUNTOOLS
	if(b->flag & BI_OFFSCREEN) {
		p = sub(p, b->rect.origin);
		bit = (pr_get((Pixrect *)b->dr, p.x, p.y) != 0);
	} else
		bit = (pw_get((Pixwin *)b->dr, p.x, p.y) != 0);
#endif SUNTOOLS
	return (bit);
}
