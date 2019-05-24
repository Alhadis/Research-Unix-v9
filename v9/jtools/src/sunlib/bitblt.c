/* bitblt routines */
#include "jerq.h"

void
bitblt (sb, r, db, p, f)
Bitmap *sb, *db;
Rectangle r;	/* in source bitmap */
Point p;	/* in dest bitmap */
Code f;
{
	int wd = r.corner.x - r.origin.x;
	int ht = r.corner.y - r.origin.y;

	if(sb->flag & BI_OFFSCREEN)
		r.origin = sub(r.origin, sb->rect.origin);
	if(db->flag & BI_OFFSCREEN)
		p = sub(p, db->rect.origin);
#ifdef X11
	XSetFunction(dpy, gc, f);
	XCopyArea(dpy, sb->dr, db->dr, gc, r.origin.x, r.origin.y,
		wd, ht, p.x, p.y);
#endif X11
#ifdef SUNTOOLS
	if(sb->flag & BI_OFFSCREEN){
		if(db->flag & BI_OFFSCREEN)	/* pr to pr */
			pr_rop((Pixrect *)db->dr, p.x, p.y, wd, ht, f,
				(Pixrect *)sb->dr, r.origin.x, r.origin.y);
		else				/* pr to pw */
			pw_write((Pixwin *)db->dr, p.x, p.y, wd, ht, f,
				(Pixrect *)sb->dr, r.origin.x, r.origin.y);
	}
	else{
		if(db->flag & BI_OFFSCREEN)	/* pw to pr */
			pw_read((Pixrect *)db->dr, p.x, p.y, wd, ht, f,
				(Pixwin *)sb->dr, r.origin.x, r.origin.y);
		else				/* pw to pw */
			pw_copy((Pixwin *)db->dr, p.x, p.y, wd, ht, f,
				(Pixwin *)sb->dr, r.origin.x, r.origin.y);
	}
#endif SUNTOOLS
}

void
point (b, p, f)
Bitmap *b;
Point p;
Code f;
{
	int i;

	if(b->flag & BI_OFFSCREEN)
		p = sub(p, b->rect.origin);
#ifdef X11
	XSetFunction(dpy, gc, f);
	XDrawPoint(dpy, b->dr, gc, p.x, p.y);
#endif X11
#ifdef SUNTOOLS
	switch(f){
	case F_STORE:
	case F_OR:
		if(b->flag & BI_OFFSCREEN)
			pr_put((Pixrect *)b->dr, p.x, p.y, 1);
		else
			pw_put((Pixwin *)b->dr, p.x, p.y, 1);
		break;
	case F_CLR:
		if(b->flag & BI_OFFSCREEN)
			pr_put((Pixrect *)b->dr, p.x, p.y, 0);
		else
			pw_put((Pixwin *)b->dr, p.x, p.y, 0);
		break;
	case F_XOR:
		if(b->flag & BI_OFFSCREEN) {
			i = pr_get((Pixrect *)b->dr,p.x,p.y);
			pr_put((Pixrect *)b->dr, p.x, p.y, !i);
		} else {
			i = pw_get((Pixwin *)b->dr,p.x,p.y);
			pw_put((Pixwin *)b->dr, p.x, p.y, !i);
		}
		break;
	}
#endif SUNTOOLS
}

void
rectf (b, r, f)
Bitmap *b;
Rectangle r;
Code f;
{
	Point diff;

	diff = sub(r.corner, r.origin);
	if(b->flag & BI_OFFSCREEN)
		r.origin = sub(r.origin, b->rect.origin);
#ifdef X11
	XSetFunction(dpy, gc, f);
	XFillRectangle(dpy, b->dr, gc,r.origin.x,r.origin.y,diff.x,diff.y);
#endif X11
#ifdef SUNTOOLS
	switch(f){
	case F_STORE:
	case F_OR:
		f = PIX_NOT(PIX_SRC);
		break;
	case F_CLR:
		f = PIX_SRC;
		break;
	case F_XOR:
		f = PIX_NOT(PIX_SRC) ^ PIX_DST;
		break;
	}
	if(b->flag & BI_OFFSCREEN)
		pr_rop((Pixrect *)b->dr, r.origin.x, r.origin.y,
			diff.x, diff.y, f, 0, 0, 0);
	else
		pw_write((Pixwin *)b->dr, r.origin.x, r.origin.y,
			diff.x, diff.y, f, 0, 0, 0);
#endif SUNTOOLS
}

screenswap (bp, rect, screenrect)
register Bitmap *bp;
Rectangle rect;
Rectangle screenrect;
{
	bitblt(&display, screenrect, bp, rect.origin, F_XOR);
	bitblt(bp, rect, &display, screenrect.origin, F_XOR);
	bitblt(&display, screenrect, bp, rect.origin, F_XOR);
}

void
segment (b, p, q, f)
Bitmap *b;
Point p, q;
Code f;
{
	int i;

	if(b->flag & BI_OFFSCREEN){
		p = sub(p, b->rect.origin);
		q = sub(q, b->rect.origin);
	}
#ifdef X11
	XSetFunction(dpy, gc, f);
	XDrawLine(dpy, b->dr, gc, p.x, p.y, q.x, q.y);
#endif X11
#ifdef SUNTOOLS
	/*	Blit compatability - don't set the last pixel	*/
	if(b->flag & BI_OFFSCREEN){
		i = pr_get((Pixrect *)b->dr,q.x,q.y);
		pr_vector((Pixrect *)b->dr, p.x, p.y, q.x, q.y, f, 1);
		pr_put((Pixrect *)b->dr, q.x, q.y, i);
	}
	else{
		i = pw_get((Pixwin *)b->dr,q.x,q.y);
		pw_vector((Pixwin *)b->dr, p.x, p.y, q.x, q.y, f, 1);
		pw_put((Pixwin *)b->dr, q.x, q.y, i);
	}
#endif SUNTOOLS
}

void
texture (b, r, tile, f)
Bitmap *b;
Rectangle r;
Texture *tile;
Code f;
{
#ifdef SUNTOOLS
	extern struct pixrectops mem_ops;
	static struct mpr_data d = 
		{mpr_linebytes(16,1), (short *)0, {0, 0}, 0, 0};
	static struct pixrect textrect = {&mem_ops, 16, 16, 1, (caddr_t)&d};
#endif SUNTOOLS
	Point diff;

	diff = sub(r.cor, r.org);
	if (b->flag & BI_OFFSCREEN)
		r.org = sub(r.org, b->rect.org);
#ifdef X11
	XSetFunction(dpy, gc, f);
	XSetFillStyle(dpy, gc, FillTiled);
	XSetTile(dpy, gc, *tile);
	XFillRectangle(dpy, b->dr, gc, r.org.x, r.org.y, diff.x, diff.y);
	XSetFillStyle(dpy, gc, FillSolid);
#endif X11
#ifdef SUNTOOLS
	d.md_image = *tile;
	if(b->flag & BI_OFFSCREEN)
		pr_replrop((Pixrect *)b->dr, r.origin.x, r.origin.y,
			diff.x, diff.y, f, &textrect, r.origin.x, r.origin.y);
	else
		pw_replrop((Pixwin *)b->dr, r.origin.x, r.origin.y,
			diff.x, diff.y, f, &textrect, r.origin.x, r.origin.y);
#endif SUNTOOLS
}

void
texture32 (b, r, tile, c)
Bitmap *b;
Rectangle r;
Texture32 *tile;
Code c;
{
#ifdef SUNTOOLS
	extern struct pixrectops mem_ops;
	static struct mpr_data d = 
		{mpr_linebytes(32,1), (short *)0, {0, 0}, 0, 0};
	static struct pixrect textrect32 =
		{&mem_ops, 32, 32, 1, (caddr_t)&d};
#endif SUNTOOLS
	Point diff;

	diff = sub(r.cor, r.org);
	if (b->flag & BI_OFFSCREEN)
		r.org = sub(r.org, b->rect.org);
#ifdef X11
	XSetFunction(dpy, gc, c);
	XSetFillStyle(dpy, gc, FillTiled);
	XSetTile(dpy, gc, *tile);
	XFillRectangle(dpy, b->dr, gc, r.org.x, r.org.y, diff.x, diff.y);
	XSetFillStyle(dpy, gc, FillSolid);
#endif X11
#ifdef SUNTOOLS
	d.md_image = (short *)*tile;
	if(b->flag & BI_OFFSCREEN)
		pr_replrop((Pixrect *)b->dr, r.origin.x, r.origin.y,
		   diff.x, diff.y, c, &textrect32, r.origin.x, r.origin.y);
	else
		pw_replrop((Pixwin *)b->dr, r.origin.x, r.origin.y,
		   diff.x, diff.y, c, &textrect32, r.origin.x, r.origin.y);
#endif SUNTOOLS
}
