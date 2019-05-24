#include <jerq.h>

extern Bitmap *bp;
extern Rectangle rect, windowrect();
extern Cursor deadmouse, target;

Rectangle
findproc(flag)
{
	Rectangle r;
	Point mpos;

	cursswitch(&target);
	Jscreengrab();
	while(!button123())
		wait(MOUSE);
	mpos = add(mouse.xy, Joffset);
	while(button123())
		wait(MOUSE);
	Jscreenrelease();
	cursswitch(&deadmouse);
	return windowrect(mpos, flag);
}

Rectangle
kbdrect()
{
	extern Point kbdp;
	Rectangle r;
	r=Drect; r.origin.y=r.corner.y-fontheight(&defont)-4;
	kbdp=add(r.origin,Pt(2,3));
	return r;
}

checkshape(flag)
{
	Rectangle r, s;
	if (flag || (P->state & RESHAPED)) {
		P->state &= ~RESHAPED;
		r = kbdrect();
		s = Drect; s.corner.y = r.origin.y;
		rectf(&display, r, F_STORE);
	}
}

flash(r)
Rectangle r;
{
	if (r.corner.x > r.origin.x && r.corner.y > r.origin.y) {
		bp = &Jfscreen;
		rect = r;
	} else
		bp = (Bitmap *)0;
	if (bp) {
		visible(0);
		Jscreengrab();
		rectf(bp, r, F_XOR); sleep(20); rectf(bp, r, F_XOR);
		Jscreenrelease();
		visible(1);
	}
}

visible(flag)
{
/*
	static Rectangle prevrect;
	static int state = 1;
	if (state == flag)
		return;
	else if (flag)
		state = 1, reshape(prevrect), checkshape(1);
	else if (bp && rectXrect(P->layer->rect, rect))
		state = 0, prevrect = P->layer->rect, reshape(Rect(0,0,0,0));
*/
}
