#include	<jerq.h>
Texture boxcurs = {
	0x43FF, 0xE001, 0x7001, 0x3801, 0x1D01, 0x0F01, 0x8701, 0x8F01,
	0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0xFFFF,
};
Rectangle
canon(p1, p2)
	Point p1, p2;
{
	Rectangle r;
	r.origin.x = min(p1.x, p2.x);
	r.origin.y = min(p1.y, p2.y);
	r.corner.x = max(p1.x, p2.x);
	r.corner.y = max(p1.y, p2.y);
	return(r);
}
Rectangle
getrectb(n)
	int n;
{
	Rectangle r;
	Texture *t;
	Point p1, p2;
	t = cursswitch(&boxcurs);
	while(button123())nap(1);
	while(!button123()) nap(1);
	if(!(mouse.buttons&n)){
		r.origin.x=r.origin.y=r.corner.x=r.corner.y=0;
		while(button123()) nap(1);
		goto Return;
	}
	p1=mouse.xy;
	p2=p1;
	r=canon(p1, p2);
	outline(r);
	for(; mouse.buttons&n; nap(2)){
		outline(r);
		p2=mouse.xy;
		r=canon(p1, p2);
		outline(r);
	}
	outline(r);	/* undraw for the last time */
    Return:
	(void)cursswitch(t);
	return r;
}
Rectangle
getrect(n)
{
	return getrectb(8>>n);
}
outline(r)
	Rectangle r;
{
	segment(&display,r.origin,Pt(r.corner.x,r.origin.y),F_XOR);
	segment(&display,Pt(r.corner.x,r.origin.y),r.corner,F_XOR);
	segment(&display,r.corner,Pt(r.origin.x,r.corner.y),F_XOR);
	segment(&display,Pt(r.origin.x,r.corner.y),r.origin,F_XOR);
}
