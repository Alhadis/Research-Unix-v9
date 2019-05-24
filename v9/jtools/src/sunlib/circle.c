
#include "jerq.h"

/*	jerq routines: circle, disc, ellipse, eldisc, arc	*/

/*	circle:
	Form a circle of radius r centered at x1,y1
	The boundary is a sequence of vertically, horizontally,
	or diagonally adjacent points that minimize 
	abs(x^2+y^2-r^2).
	The circle is guaranteed to be symmetric about
	the horizontal, vertical, and diagonal axes
 */
circle (b, p, r, f)
Bitmap *b;
Point p;
{
	int x1=p.x;
	register y1 = p.y;
	register eps = 0;	/* x^2 + y^2 - r^2 */
	register dxsq = 1;	/* (x+dx)^2-x^2*/
	register dysq = 1 - 2*r;
	register exy;
	int x0 = x1;
	register y0 = y1 - r;
	y1 += r;
	initpoints(b, f);
	if(f == F_XOR){		/* endpoints coincide */
		points(Pt(x0,y0));
		points(Pt(x0,y1));
	}
	while(y1 > y0){
		points(Pt(x0,y0));
		points(Pt(x0,y1));
		points(Pt(x1,y0));
		points(Pt(x1,y1));
		exy = eps + dxsq + dysq;
		if(-exy <= eps + dxsq){
			y1--;
			y0++;
			eps += dysq;
			dysq += 2;
		}
		if(exy <= -eps){
			x1++;
			x0--;
			eps += dxsq;
			dxsq += 2;
		}
	}
	points(Pt(x0,y0));
	points(Pt(x1,y0));
	endpoints();
}

/*	Fill a disc of radius r centered at x1,y1
	The boundary is a sequence of vertically, horizontally,
	or diagonally adjacent points that minimize 
	abs(x^2+y^2-r^2).
	The circle is guaranteed to be symmetric about
	the horizontal, vertical, and diagonal axes
*/
disc (b, p, r, f)
Bitmap *b;
Point p;
int r;
int f;
{
	register x1, y1;
	register eps = 0;		/* x^2 + y^2 - r^2 */
	int dxsq = 1;			/* (x+dx)^2-x^2 */
	int dysq = 1 - 2 * r;
	int exy;
	register x0;
	register y0;

	if(b->flag & BI_OFFSCREEN)
		p = sub(p, b->rect.origin);
	x1 = p.x;
	y1 = p.y;
	x0 = x1;
	y0 = y1 - r;
	x1++;			/* to offset jerq's half-open lines */
	y1 += r;
	while(y1 > y0){
		exy = eps + dxsq + dysq;
		if(-exy <= eps + dxsq){
			rectf(b, Rect(x0, y0, x1, y0+1), f);
			rectf(b, Rect(x0, y1, x1, y1+1), f);
			y1--;
			y0++;
			eps += dysq;
			dysq += 2;
		}
		if(exy <= -eps){
			x1++;
			x0--;
			eps += dxsq;
			dxsq += 2;
		}
	}
	rectf(b, Rect(x0, y0, x1, y0+1), f);
}

/*	arc:
	draw an approximate arc centered at x0,y0 of an
	integer grid and running anti-clockwise from
	x1,y1 to the vicinity of x2,y2.
	If the endpoints coincide, draw a complete circle.
	The "arc" is a sequence of vertically, horizontally,
	or diagonally adjacent points that minimize 
	abs(x^2+y^2-r^2).
	The circle is guaranteed to be symmetric about
	the horizontal, vertical, and diagonal axes
*/
#define sq(x)		((long)(x)*(x))
#define	sgn(x)		((x) < 0 ? -1 : (x)==0 ? 0 : 1)
#define mabs(x) 	(x < 0 ? -x : x)

static
Point
nearby (p1, p2)		/* called by arc()	*/
Point p1, p2;
{
	long eps, exy;	/*integers but many bits*/
	int d;
	register dy;
	register dx;
	register x1 = p1.x;
	register y1 = p1.y;
	register x2 = p2.x;
	register y2 = p2.y;
	eps = sq(x2) + sq(y2) - sq(x1) - sq(y1);
	d = eps>0? -1: 1;
	for( ; ; eps=exy, x2+=dx, y2+=dy){
		if(abs(y2) > abs(x2)){
			dy = d*sgn(y2);
			dx = 0;
		}
		else{
			dy = 0;
			dx = d*sgn(x2);
			if(dx==0)
				dx = 1;
		}
		exy = eps + (2*x2+dx)*dx + (2*y2+dy)*dy;
		if(mabs(eps) <= mabs(exy))
			break;
	}
	p2.x = x2;
	p2.y = y2;
	return(p2);
}

arc (bp, p0, p2, p1, f)
register Bitmap *bp;
Point p0, p1, p2;
{
	register dx, dy;
	register eps;	/* x^2 + y^2 - r^2 */
	int dxsq, dysq;	/* (x+dx)^2-x^2, ...*/
	int ex, ey, exy;
	p1 = sub(p1, p0);
	p2 = sub(p2, p0);
	p2 = nearby(p1, p2);
	dx = -sgn(p1.y);	/* y1==0 is soon fixed */
	dy = sgn(p1.x);
	dxsq = (2*p1.x + dx)*dx;
	dysq = (2*p1.y + dy)*dy;
	eps = 0;
	initpoints(bp, f);
	do{
		if(p1.x == 0){
			dy = -sgn(p1.y);
			dysq = (2*p1.y + dy)*dy;
		}
		else if(p1.y == 0){
			dx = -sgn(p1.x);
			dxsq = (2*p1.x + dx)*dx;
		}
		ex = abs(eps + dxsq);
		ey = abs(eps + dysq);
		exy = abs(eps + dxsq + dysq);
		if(ex<ey || exy<=ey){
			p1.x += dx;
			eps += dxsq;
			dxsq += 2;
		}
		if(ey<ex || exy<=ex){
			p1.y += dy;
			eps += dysq;
			dysq += 2;
		}
		points(Pt(p0.x+p1.x, p0.y+p1.y));
	} while(!(p1.x==p2.x && p1.y==p2.y));
/*	the equality end test is justified
	because it is impossible that
	abs(x^2+y^2-r^2)==abs((x++-1)^2+y^2-r^2) or
	abs(x^2+y^2-r^2)==abs(x^2+(y++-1)-r^2),
	and no values of x or y are skipped.	*/
	endpoints();
}

ellipse (bp, p, a, b, f)
Bitmap *bp;
Point p;
long a, b;
Code f;
{
	if(a==0 || b==0)
		segment(bp, Pt(p.x-a, p.y-b), Pt(p.x+a, p.y+b), f);
	else
		ellip2(bp, p, a, b, Pt(0, b), Pt(0, b), f);
}

#define		BIG		077777
#define		HUGE		07777777777L
/*
	resid: calculate b*b*x*x + a*a*y*y - a*a*b*b avoiding ovfl
		called by ellip1() and ellip2()
*/
static long
resid (a,b,x,y)
register long a, b;
long x, y;
{
	long e = 0;
	long u = b*(a*a - x*x);
	long v = a*y*y;
	register q = u>BIG? HUGE/u: BIG;
	register r = v>BIG? HUGE/v: BIG;
	while(a || b){
		if(e>=0 && b){
			if(q>b) q = b;
			e -= q*u;
			b -= q;
		}
		else{
			if(r>a) r = a;
			e += r*v;
			a -= r;
		}
	}
	return(e);
}

#define		labs(x,y)	if((x=y)<0) x = -x
#define		samesign(x,y)	(((int)(x)^(int)(y)) > 0)

ellip2 (bp, p0, a, b, p1, p2, f)
Point p0, p1, p2;
long a, b;
register Bitmap *bp;
Code f;
{
	int dx = p1.y>0 ? 1 : p1.y<0 ? -1 : p1.x>0 ? -1 : 1;
	int dy = p1.x>0 ? -1 : p1.x<0 ? 1 : p1.y>0 ? -1 : 1;
	long a2 = a*a;
	long b2 = b*b;
	register long dex = b2*(2*dx*p1.x+1);
	register long e;
	register long dey = a2*(2*dy*p1.y+1);
	register long ex, ey, exy;
	int partial = !eqpt(p1, p2);

	if(partial &&
	   (p1.x==0 && p2.x==0 && samesign(p1.y, p2.y) ||
	    p1.y==0 && p2.y==0 && samesign(p1.x, p2.x))) {
		segment(bp, add(p0, p1), add(p0,p2), f);
		return;
	}
	e = resid(a, b, p1.x, p1.y);
	a2 *= 2;
	b2 *= 2;
	initpoints(bp, f);
	do{
		labs(ex, e+dex);
		labs(ey, e+dey);
		labs(exy, e+dex+dey);
		if(exy<=ex || ey<ex){
			p1.y += dy;
			e += dey;
			dey += a2;
		}
		if(exy<=ey || ex<ey){
			p1.x += dx;
			e += dex;
			dex += b2;
		}
		if(p1.x == 0){
			if(abs(p1.y) == b){
				dy = -dy;
				dey = -dey + a2;
				partial = 0;
			}
			/* don't double-draw skinny ends */
			else if(!samesign(p1.y, dy) && !partial)
				continue;
		}
		else if(p1.y == 0){
			if(abs(p1.x) == a){
				dx = -dx;
				dex = -dex + b2;
				partial = 0;
			}
			else if(!samesign(p1.x, dx) && !partial)
				continue;
		}
		points(add(p0, p1));
	} while(! eqpt(p1, p2));
	endpoints();
}

ellip1 (bp, p0, a, b, action, p1, p2, f)
Bitmap *bp;
Point p0, p1, p2;
long a, b;
register void (*action) ();
int f;
{
	int dx = p1.y > 0 ? 1 : p1.y < 0 ? -1 : p1.x > 0 ? -1 : 1;
	int dy = p1.x > 0 ? -1 : p1.x < 0 ? 1 : p1.y > 0 ? -1 : 1;
	long a2 = a * a;
	long b2 = b * b;
	register long dex = b2 * (2 * dx * p1.x + 1);
	register long e;
	register long dey = a2 * (2 * dy * p1.y + 1);
	register long ex, ey, exy;
	int partial = !eqpt (p1, p2);
	
	if(partial &&
	    (p1.x == 0 && p2.x == 0 && samesign (p1.y, p2.y) ||
	     p1.y == 0 && p2.y == 0 && samesign (p1.x, p2.x))){
		segment(bp, add (p0, p1), add (p0, p2), f);
		return;
	}
	e = resid(a, b, p1.x, p1.y);
	a2 *= 2;
	b2 *= 2;
	do{
		labs(ex, e + dex);
		labs(ey, e + dey);
		labs(exy, e + dex + dey);
		if(exy <= ex || ey < ex){
			p1.y += dy;
			e += dey;
			dey += a2;
		}
		if(exy <= ey || ex < ey){
			p1.x += dx;
			e += dex;
			dex += b2;
		}
		if(p1.x == 0){
			if(abs(p1.y) == b){
				dy = -dy;
				dey = -dey + a2;
				partial = 0;
			}
			else
				if(!samesign (p1.y, dy) && !partial)
					continue;
			/* don't double-draw skinny ends */
		}
		else
			if(p1.y == 0){
				if(abs (p1.x) == a){
					dx = -dx;
					dex = -dex + b2;
					partial = 0;
				}
				else
					if(!samesign (p1.x, dx) && !partial)
						continue;
			}
		(*action)(bp, add (p0, p1), f);
	} while(!eqpt (p1, p2));
}

static int yaxis;	/* used in scan and eldisc	*/
static int xaxis;	/* used in scan and eldisc	*/
static Point lp;	/* used in scan and eldisc	*/

static
void
scan (bp, p, f)		/* called by eldisc() and ellip1()	*/
Bitmap *bp;
Point p;
Code f;
{
	register x, y;

	if((p.y != lp.y) && (lp.y != -1)){
		x = xaxis - lp.x;
		y = yaxis - lp.y;
		rectf(bp, Rect(lp.x, lp.y, x+1, lp.y+1), f);
		rectf(bp, Rect(lp.x, y, x+1, y+1), f);
	}
	lp = p;
}

eldisc (bp, p, a, b, f)
Bitmap *bp;
Point p;
int a, b;
Code f;
{
	register x0 = p.x;
	register y0 = p.y;

	yaxis = 2*p.y;
	xaxis = 2*p.x;
	lp.y = -1;
	if(a==0 || b==0)
		segment(bp, Pt(x0-a,y0-b), Pt(x0+a,y0+b), f);
	else{
		ellip1(bp, p, a, b, scan, Pt(0, -b), Pt(-a, 0), f);
		scan(bp, Pt(0, -1), f);
		rectf(bp, Rect(p.x-a, p.y, p.x+a+1, p.y+1), f);
	}
}

/*	elarc routines	*/

static struct dist {
	Point s;
	Point m;
	long e;
} d1, d2;

static
void
test(x, p)		/* called by survet()	*/
Point x;
register struct dist *p;
{
	register long dx = x.x - p->s.x;
	register long dy = x.y - p->s.y;
	register long e = dx*dx+dy*dy;

	if(e <= p->e){
		p->m = x;
		p->e = e;
	}
}

static
void
survey(bp, x, f)	/* called by elarc()	*/
Bitmap *bp;
Point x;
Code f;
{
	test(x, &d1);
	test(x, &d2);
}

#define HUGE2		017777777777
#define	sgn2(x)	((x)<0? -1 : (x)==0? 0 : 1)

/*
	elarc(bp,p0,a,b,p1,p2,f) draws in bitmap bp an arc of the ellipse
	centered at p0 with half-axes a,b extending counterclockwise
	from a point near p1 to a point near p2
	args reversed because ellip1 draws clockwise 
*/
elarc (bp, p0, a, b, p1, p2, f)
Bitmap *bp;
Point p0, p1, p2;
Code f;
{
	if(a==0)
		segment(bp, Pt(p0.x, p1.y), Pt(p0.x, p2.y), f);
	else if(b==0)
		segment(bp, Pt(p1.x, p0.y), Pt(p2.x, p0.y), f);
	else{
		int sx1;
		int sy1;
		int sx2;
		int sy2;

		d1.s = sub(p1, p0);
		d2.s = sub(p2, p0);
		sx1 = sgn2(d1.s.x);
		sy1 = sgn2(d1.s.y);
		sx2 = sgn2(d2.s.x);
		sy2 = sgn2(d2.s.y);
		d1.s.x *= sx1;
		d1.s.y *= sy1;
		d2.s.x *= sx2;
		d2.s.y *= sy2;
		d1.e = d2.e = HUGE2;
		survey(bp, Pt(0, b), f);
		ellip1(bp, Pt(0, 0), a, b, survey, Pt(0,b), Pt(a, 0), f);
		if(!eqpt(d1.m, d2.m))
			point(bp, d1.m, f);
		ellip1(bp, p0, a, b, point,
			Pt(d1.m.x*sx1, d1.m.y*sy1),
			Pt(d2.m.x*sx2, d2.m.y*sy2), f);
	}
}

/*
	dak's points routines for buffering X calls
*/
#ifdef	X11
#define PBSIZE	100
#define flushpt()	if (xpcnt) flushpoints();
static XPoint xp[PBSIZE];
static xpcnt;
#endif X11
static Code fc;
static ispixmap;
static Bitmap *bitm;

points (p)
Point p;
{
#ifdef SUNTOOLS
	point(bitm,p,fc);
#endif SUNTOOLS
#ifdef X11
	register XPoint *x;

	if(ispixmap)
		p = sub(p, bitm->rect.origin);
	x = &xp[xpcnt];
	x->x = p.x;
	x->y = p.y;
	if (++xpcnt == PBSIZE)
		flushpoints();
#endif X11
}

initpoints (b, f)
Bitmap *b;
Code f;
{
	if(b->flag & BI_OFFSCREEN)
		ispixmap = 1;
	else {
#ifdef SUNTOOLS
		struct rect lkrect;
		if(!(b->flag & BI_OFFSCREEN)){
			win_getsize(displayfd, &lkrect);
			pw_lock((Pixwin *)b->dr, &lkrect);
		}
#endif SUNTOOLS
		ispixmap = 0;
	}
	bitm = b;
#ifdef X11
	XSetFunction(dpy, gc, f);
#endif X11
	fc = f;
}

endpoints()
{
#ifdef SUNTOOLS
	if(!ispixmap)
		pw_unlock((Pixwin *)bitm->dr);
#endif SUNTOOLS
#ifdef X11
	flushpt();
#endif X11
}

#ifdef X11
flushpoints()
{
	if(xpcnt){
		XDrawPoints(dpy, bitm->dr, gc, xp, xpcnt, CoordModeOrigin);
		xpcnt = 0;
	}
}
#endif X11
