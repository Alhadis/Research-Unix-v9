#include "jerq.h"
/* These routines are NOT portable, but they are fast. */

Point
add (a, b)
Point a, b;
{
	register short *ap= &a.x, *bp= &b.x;
	*ap++ += *bp++;
	*ap += *bp;
	return a;
}

Point
sub (a, b)
Point a, b;
{
	register short *ap= &a.x, *bp= &b.x;
	*ap++ -= *bp++;
	*ap -= *bp;
	return a;
}

Point
mul (a, b)
Point a;
register b;
{
	register short *ap= &a.x;	
	*(ap++)*=b;
	*ap*=b;
	return a;
}

Point
div (a, b)
Point a;
register b;
{
	register short *ap= &a.x;
	*(ap++)/=b;
	*ap/=b;
	return a;
}

eqpt (p, q)
Point p, q;
{
	register long *pp=(long *)&p, *qq=(long *)&q;
	return *pp==*qq;
}

eqrect (r, s)
Rectangle r, s;
{
	register long *rr=(long *)&r, *ss=(long *)&s;
	return *rr++==*ss++ && *rr==*ss;
}

Rectangle
inset (r,n)
Rectangle r;
register n;
{
	register short *rp= &r.origin.x;
	*rp++ += n;
	*rp++ += n;
	*rp++ -= n;
	*rp   -= n;
	return r;
}

/* muldiv is a macro in jerq.h	*/

ptinrect (p, r)
Point p;
Rectangle r;
{
	return(p.x >= r.origin.x && p.x < r.corner.x
	    && p.y >= r.origin.y && p.y < r.corner.y);
}

Rectangle
raddp (r, p)
Rectangle r;
Point p;
{
	register short *rp= &r.origin.x, *pp= &p.x;
	*rp++ += *pp++;
	*rp++ += *pp--;
	*rp++ += *pp++;
	*rp   += *pp;
	return r;
}

Rectangle
rsubp (r, p)
Rectangle r;
Point p;
{
	register short *rp= &r.origin.x, *pp= &p.x;
	*rp++ -= *pp++;
	*rp++ -= *pp--;
	*rp++ -= *pp++;
	*rp   -= *pp;
	return r;
}

rectXrect(r, s)
Rectangle r, s;
{
#define c corner
#define o origin
	return(r.o.x<s.c.x && s.o.x<r.c.x && r.o.y<s.c.y && s.o.y<r.c.y);
}

rectclip (rp, b)	/* first by reference, second by value */
register Rectangle *rp;
Rectangle b;
{
	register Rectangle *bp= &b;
	/*
	 * Expand rectXrect() in line for speed
	 */
	if((rp->o.x<bp->c.x && bp->o.x<rp->c.x &&
	    rp->o.y<bp->c.y && bp->o.y<rp->c.y)==0)
		return 0;
	/* They must overlap */
	if(rp->origin.x<bp->origin.x)
		rp->origin.x=bp->origin.x;
	if(rp->origin.y<bp->origin.y)
		rp->origin.y=bp->origin.y;
	if(rp->corner.x>bp->corner.x)
		rp->corner.x=bp->corner.x;
	if(rp->corner.y>bp->corner.y)
		rp->corner.y=bp->corner.y;
	return 1;
}
