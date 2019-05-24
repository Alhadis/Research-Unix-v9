#if defined(sparc)
#include "jerq.h"

Point Pt(x,y)
{
	Point p;

	p.x = x;
	p.y = y;
	return p;
}

Rectangle Rect(x1,y1,x2,y2)
{
	Rectangle r;

	r.origin.x = x1;
	r.origin.y = y1;
	r.corner.x = x2;
	r.corner.y = y2;
	return r;
}

Rectangle Rpt(p1, p2)
Point p1, p2;
{
	Rectangle r;

	r.origin.x = p1.x;
	r.origin.y = p1.y;
	r.corner.x = p2.x;
	r.corner.y = p2.y;
	return r;
}
#endif
