#include <jerq.h>
#include "event.h"
#include "rock.h"
typedef struct{
	Point	x;
	Rectangle r;
	Point v;
	char	allocated;
	char	nmoves;
}Bomb;
#define	NBOMB	16	/* known in event type */
#define	BOMBTICKS	20
#define	VBOMB		15
#define	RADBOMB		2
static int NMOVES;
Bomb bomb[NBOMB];
int	bombadvance();
initbomb()
{
	register Bomb *b;
	NMOVES=((Drect.corner.y/2)/VBOMB);
	for(b=bomb; b<&bomb[NBOMB]; b++)
		b->allocated=0;
}
startbomb(p, v, dir)
	Point p, v;
	register dir;
{
	register i;
	register Bomb *b;
	for(i=0, b=bomb; i<NBOMB; i++, b++)
		if(b->allocated==0){
			if(addevent(newevent(), BOMBTICKS, bombadvance, EBOMB|i)){
				b->x=p;
				b->r.origin=add(p, Pt(-1, -1));
				b->r.corner=add(p, Pt(1, 1));
				b->nmoves=NMOVES;
				b->v=v;
				b->v.x+=muldiv(VBOMB, cos(dir), 1024);
				b->v.y+=muldiv(VBOMB, sin(dir), 1024);
				b->allocated=1;
				jrectf(b->r, F_XOR);
			}
			break;
		}
}
bombadvance(x)
{
	register Bomb *b= &bomb[x&=~EBOMB];
	register Rock *r;
	extern notdrawn;
	jrectf(b->r, F_XOR);
	if(--b->nmoves==0){
   ReturnNo:
		b->allocated=0;
		return 0;
	}
	r=collision(b->x, hash(b->x), RADBOMB);
	if(r){
		explode(r->x);
		split(r);
		killrock(r);
		goto ReturnNo;
	}
	b->x.x+=b->v.x;
	b->x.y+=b->v.y;
	onscreen(&b->x);
	b->r.origin=add(b->x, Pt(-1, -1));
	b->r.corner=add(b->x, Pt(1, 1));
	jrectf(b->r, F_XOR);
	if(notdrawn > -20)
		notdrawn-=2;
	return 1;
}

Point
transform(p)
	Point p;
{
#	define	o Drect.origin
#	define	c Drect.corner
	p.x=muldiv(c.x-o.x, p.x, Drect.corner.x)+o.x;
	p.y=muldiv(c.y-o.y, p.y, Drect.corner.y)+o.y;
#undef	o
#undef	c
	return p;
}
Rectangle
rtransform(r)
	Rectangle r;
{
	r.origin=transform(r.origin);
	r.corner=transform(r.corner);
	return r;
}

jrectf(r, fc)
	Rectangle r;
{
	rectf(&display, rtransform(r), fc);
}
