#define bool  int
#define true  1
#define false 0

#define NOHIT    -1

#define I_NULL   -1
#define I_OR      0
#define I_STORE   1
#define I_CLR     2
#define I_XOR     3
#define I_AND     4

#include "jerq.h"
#include <stdio.h>

#define SPACING 24

#define sgn(x) ((x)<0 ? -1 : (x)==0 ? 0 : 1)

int XSIZE = 50, YSIZE = 50;

int horsize(r)
Rectangle r;
{
   return(r.corner.x - r.origin.x);
}

int versize(r)
Rectangle r;
{
   return(r.corner.y - r.origin.y);
}

extern Texture *(imenu[]);

int lasthitx = 0, lasthity = 0;
#define SRC_ID     0
#define SRC_CLR    1
char *blitsrctext[] = 
   {"src := src","src := 0",NULL};

#define DST_STORE  0
#define DST_OR     1
#define DST_XOR    2
#define DST_AND    3
#define DST_CLR    4

char *blitdsttext[] = 
   {"dst := src","dst := src or dst","dst := src xor dst","dst := src and dst","dst := 0",NULL};

Menu blitsrcmenu = {blitsrctext};
Menu blitdstmenu = {blitdsttext};

short Cmove_bits[] = {
	 0x0000, 0x0000, 0x00C0, 0x00E0,
	 0x00F0, 0x7FF8, 0x7FDC, 0x600E,
	 0x6007, 0x600E, 0x7FDC, 0x7FF8,
	 0x00F0, 0x00E0, 0x00C0, 0x0000,
};
Texture Cmove;

short  Ccopy_bits[] = {
	 0x0000, 0x7FE0, 0x4020, 0x4020,
	 0x473F, 0x4FA1, 0x4DA1, 0x4C39,
	 0x4FBD, 0x472D, 0x4021, 0x403D,
	 0x7FF9, 0x0201, 0x0201, 0x03FF,
};
Texture Ccopy;

short Cerase_bits[] = {
	 0x03FE, 0x0556, 0x0556, 0x0AAE,
	 0x0AAE, 0x1FFC, 0x101C, 0x2028,
	 0x2028, 0x4050, 0x4050, 0xFFA0,
	 0x80A0, 0x80C0, 0x80C0, 0xFF80,
};
Texture Cerase;

short Cinvert_bits[] = {
	 0x0000, 0x0000, 0x07C0, 0x07C0,
	 0x07C0, 0x07C0, 0x07C0, 0x783C,
	 0x783C, 0x783C, 0x07C0, 0x07C0,
	 0x07C0, 0x07C0, 0x07C0, 0x0000,
};
Texture Cinvert;

short Cblit_bits[] = {
	 0x0000, 0x7FE0, 0x4020, 0x4020,
	 0x403F, 0x4021, 0x4021, 0x4021,
	 0x4021, 0x4021, 0x4021, 0x4021,
	 0x7FE1, 0x0201, 0x0201, 0x03FF,
};
Texture Cblit;

short Creflx_bits[] = {
	 0x0000, 0x0000, 0x7FFE, 0x8001,
	 0x8011, 0xC033, 0xBC5D, 0x8781,
	 0x8501, 0x8501, 0x4482, 0x3C5C,
	 0x0030, 0x0010, 0x0000, 0x0000,
};
Texture Creflx;

short Crefly_bits[] = {
	 0x03F8, 0x0424, 0x0844, 0x0844,
	 0x0844, 0x0FC4, 0x0084, 0x0384,
	 0x0484, 0x0844, 0x1024, 0x3874,
	 0x0844, 0x0844, 0x0424, 0x03F8,
};
Texture Crefly;

short Crotplus_bits[] = {
	 0x0000, 0x0000, 0x3FE0, 0x2010,
	 0x2008, 0x2008, 0x2008, 0x3F08,
	 0x0108, 0x0108, 0x070E, 0x0204,
	 0x0108, 0x0090, 0x0060, 0x0000,
};
Texture Crotplus;

short Crotminus_bits[] = {
	 0x0000, 0x0400, 0x0C00, 0x17F0,
	 0x2008, 0x4004, 0x4004, 0x2004,
	 0x1784, 0x0C84, 0x0484, 0x0084,
	 0x0084, 0x00FC, 0x0000, 0x0000,
};
Texture Crotminus;

short Cshearx_bits[] = {
	 0x0000, 0x0000, 0x0000, 0x0000,
	 0x0000, 0x03FF, 0x0000, 0x0FFC,
	 0x0000, 0x3FF0, 0x0000, 0xFFC0,
	 0x0000, 0x0000, 0x0000, 0x0000,
};
Texture Cshearx;

short Csheary_bits[] = {
	 0x0800, 0x0800, 0x0A00, 0x0A00,
	 0x0A80, 0x0A80, 0x0AA0, 0x0AA0,
	 0x0AA0, 0x0AA0, 0x02A0, 0x02A0,
	 0x00A0, 0x00A0, 0x0020, 0x0020,
};
Texture Csheary;

short Cstretch_bits[] = {
	 0x0000, 0x7CAA, 0x7CAA, 0x7CAA,
	 0x7CAA, 0x7CAA, 0x0000, 0x0000,
	 0x7CAA, 0x0000, 0x7CAA, 0x0000,
	 0x7CAA, 0x0000, 0x7CAA, 0x0000,
};
Texture Cstretch;

short Ctexture_bits[] = {
	 0x4000, 0x7000, 0xE000, 0x2000,
	 0x0444, 0x0777, 0x0EEE, 0x0222,
	 0x0444, 0x0777, 0x0EEE, 0x0222,
	 0x0444, 0x0777, 0x0EEE, 0x0222,
};
Texture Ctexture;

short Cgrid_bits[] = {
	 0x4040, 0xFFFF, 0x4040, 0x4444,
	 0x4040, 0x5555, 0x4040, 0x4444,
	 0x4040, 0xFFFF, 0x4040, 0x4444,
	 0x4040, 0x5555, 0x4040, 0x4444,
};
Texture Cgrid;

short Ccursor_bits[] = {
	 0x0000, 0x0000, 0x03E0, 0x17F0,
	 0x3FF0, 0x5FFE, 0xFFF1, 0x0421,
	 0x0002, 0x00FC, 0x0100, 0x0080,
	 0x0040, 0x0080, 0x0000, 0x0000,
};
Texture Ccursor;

short Cread_bits[] = {
	 0x0000, 0x0FFE, 0x1FFA, 0x1811,
	 0x0021, 0x8021, 0xC061, 0xC1F1,
	 0x622A, 0x3414, 0x1810, 0x0810,
	 0x0420, 0x03C0, 0x0000, 0x0000,
};
Texture Cread;

short Cwrite_bits[] = {
	 0xF000, 0xFC00, 0x6B00, 0x3580,
	 0x12C0, 0x1960, 0x0CA0, 0x06B0,
	 0x0250, 0x0358, 0x01A8, 0x00D8,
	 0x0074, 0x101C, 0xBB06, 0xEEFB,
};
Texture Cwrite;

short Cresize_bits[] = {
	 0xFF54, 0x8100, 0x8104, 0x8100,
	 0x8104, 0x8100, 0x8104, 0xFF00,
	 0x0004, 0x8000, 0x0014, 0x8038,
	 0x001D, 0xAAAF, 0x0007, 0x000F,
};
Texture Cresize;

short Chelp_bits[] = {
	 0x0000, 0x0000, 0x0000, 0x0000,
	 0xE0E0, 0x6060, 0x7F7E, 0x7DFF,
	 0x6FFB, 0x6C7B, 0x6FFE, 0xFFF8,
	 0x003C, 0x0000, 0x0000, 0x0000,
};
Texture Chelp;

short Cundo_bits[] = {
	 0x001C, 0x0022, 0x0041, 0x0081,
	 0x0101, 0x0282, 0x0544, 0x0AA8,
	 0x1550, 0x22A0, 0x4140, 0x8080,
	 0x8100, 0x8200, 0x4400, 0x3800,
};
Texture Cundo;

short white_bits[] = {
	 0x0000, 0x0000, 0x0000, 0x0000,
	 0x0000, 0x0000, 0x0000, 0x0000,
	 0x0000, 0x0000, 0x0000, 0x0000,
	 0x0000, 0x0000, 0x0000, 0x0000,
};
Cursor white;
Texture whiteT;

short menucursor_bits[] = {
	 0xFFC0, 0x8040, 0x8040, 0x8040,
	 0xFFC0, 0xFFC0, 0xFE00, 0xFEF0,
	 0x80E0, 0x80F0, 0x80B8, 0xFE1C,
	 0x800E, 0x8047, 0x8042, 0xFFC0,
};
Cursor menucursor;
Texture menucursorT;

short sweepcursor_bits[] = {
	 0x43FF, 0xE001, 0x7001, 0x3801,
	 0x1D01, 0x0F01, 0x8701, 0x8F01,
	 0x8001, 0x8001, 0x8001, 0x8001,
	 0x8001, 0x8001, 0x8001, 0xFFFF,
};
Cursor sweepcursor;
Texture sweepcursorT;

short sweeportrack_bits[] = {
	 0x43FF, 0xE001, 0x70FD, 0x3805,
	 0x1D05, 0x0F05, 0x8705, 0x8F05,
	 0xA005, 0xA005, 0xA005, 0xA005,
	 0xA005, 0xBFFD, 0x8001, 0xFFFF,
};
Cursor sweeportrack;
Texture sweeportrackT;

short clock_bits[] = {
	 0x03C0, 0x3420, 0x37E0, 0x13C0,
	 0x17F0, 0x1828, 0x2054, 0x20D4,
	 0x418A, 0x430A, 0x430A, 0x418A,
	 0x2094, 0x201C, 0x787E, 0x67F6,
};
Cursor clock;
Texture clockT;

short deadmouse_bits[] = {
	 0x0000, 0x0000, 0x0008, 0x0004,
	 0x0082, 0x0441, 0xFFE1, 0x5FF1,
	 0x3FFE, 0x17F0, 0x03E0, 0x0000,
	 0x0000, 0x0000, 0x0000, 0x0000,
};
Cursor deadmouse;
Texture deadmouseT;

initicons()
{
	Cmove = ToTexture(Cmove_bits);
	Ccopy = ToTexture(Ccopy_bits);
	Cerase = ToTexture(Cerase_bits);
	Cinvert = ToTexture(Cinvert_bits);
	Cblit = ToTexture(Cblit_bits);
	Creflx = ToTexture(Creflx_bits);
	Crefly = ToTexture(Crefly_bits);
	Crotplus = ToTexture(Crotplus_bits);
	Crotminus = ToTexture(Crotminus_bits);
	Cshearx = ToTexture(Cshearx_bits);
	Csheary = ToTexture(Csheary_bits);
	Cstretch = ToTexture(Cstretch_bits);
	Ctexture = ToTexture(Ctexture_bits);
	Cgrid = ToTexture(Cgrid_bits);
	Ccursor = ToTexture(Ccursor_bits);
	Cread = ToTexture(Cread_bits);
	Cwrite = ToTexture(Cwrite_bits);
	Cresize = ToTexture(Cresize_bits);
	Chelp = ToTexture(Chelp_bits);
	Cundo = ToTexture(Cundo_bits);
	whiteT = ToTexture(white_bits);
	white = ToCursor(white_bits, white_bits, 7, 7);
	menucursorT = ToTexture(menucursor_bits);
	menucursor = ToCursor(menucursor_bits, menucursor_bits, 7, 7);
	sweepcursorT = ToTexture(sweepcursor_bits);
	sweepcursor = ToCursor(sweepcursor_bits, sweepcursor_bits, 7, 7);
	sweeportrackT = ToTexture(sweeportrack_bits);
	sweeportrack = ToCursor(sweeportrack_bits, sweeportrack_bits, 7, 7);
	clockT = ToTexture(clock_bits);
	clock = ToCursor(clock_bits, clock_bits, 7, 7);
	deadmouseT = ToTexture(deadmouse_bits);
	deadmouse = ToCursor(deadmouse_bits, deadmouse_bits, 7, 7);
}

#define MOVE        0
#define COPY        1
#define INVERT	    2
#define ERASE	    3

#define REFLECTX    5
#define REFLECTY    6
#define ROTATEPLUS  7
#define ROTATEMINUS 8

#define SHEARX      10
#define SHEARY      11
#define STRETCH     12
#define TEXTURE     13

#define READ	    15
#define GRID        16
#define RESIZE      17
#define WRITE	    18

#define BLIT        20
#define PICK        21
#define HELP        22
#define UNDO	    23

Texture *(imenu[]) =
   {&Cmove,&Ccopy,&Cinvert,&Cerase,0,
    &Creflx,&Crefly,&Crotplus,&Crotminus,0,
    &Cshearx,&Csheary,&Cstretch,&Ctexture,0,
    &Cread,&Cgrid,&Cresize,&Cwrite,0,
    &Cblit,&Ccursor,&Chelp,&Cundo,0,
    0,0,0,0
   };


char exa[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
char buf[100],FNAME[50];
Point nullpoint, point16x16;
Rectangle icon, ICON, sweep, outl, nullrect, rect16x16;
int Xsize,Ysize;
int Xblocks,Yblocks;
int modx,divx,mody,divy;

flipring(p)
Point p;
{
   outline(&display,raddp(Rect(1,1,SPACING-2,SPACING-2),p));
   outline(&display,raddp(Rect(2,2,SPACING-3,SPACING-3),p));
}

drawimenu(xicons,yicons,r)
Rectangle r;
{
   Bitmap *textr;
   int i,j;
   rectf(&display,r,F_CLR);
   outline(&display,Rpt(r.origin,sub(r.corner,Pt(1,1))));
   textr = balloc(Rect(0,0,16,16));
   if (textr == ((Bitmap *) 0)) return(0);
   for (j=0; j<yicons; j++) {
     for (i=0; i<xicons; i++) {
       texture(textr,textr->rect,imenu[j*(xicons+1)+i],F_STORE);
       bitblt(textr,textr->rect,
              &display,
              add(r.origin,Pt(i*SPACING+(SPACING-16)/2,j*SPACING+(SPACING-16)/2)),
              F_OR);
     }
   }
   bfree(textr);
}

Point imenuhit()
{
   Bitmap *offscreen;
   Point ms,valley,diff,result,menudrift;
   Rectangle menurect;
   Cursor *oldcursor;
   int i,j,hitx,hity, ohitx,ohity;
   int xicons,yicons;
   oldcursor = cursswitch(&white);
   xicons = 0;
   for (j=0; imenu[xicons*j]; j++) {
     for (i=0; imenu[xicons*j+i]; i++);
     i++;
     xicons = i;
   }
   xicons -= 1;
   yicons = j;
 /*  while (!button3()) wait(MOUSE); */
   ms = mouse.xy;
   menurect = raddp(Rect(0,0,SPACING*xicons,SPACING*yicons),ms);
   menurect = rsubp(menurect,Pt(SPACING*lasthitx,SPACING*lasthity));
   menudrift.x = min(menurect.corner.x,Drect.corner.x);
   menudrift.y = min(menurect.corner.y,Drect.corner.y);
   menudrift = sub(menudrift,menurect.corner);
   menurect = raddp(menurect,menudrift); cursset(ms=add(ms,menudrift));
   menudrift.x = max(menurect.origin.x,Drect.origin.x);
   menudrift.y = max(menurect.origin.y,Drect.origin.y);
   menudrift = sub(menudrift,menurect.origin);
   menurect = raddp(menurect,menudrift); cursset(add(ms,menudrift));
   offscreen = balloc(menurect);
   if (offscreen != ((Bitmap *) 0)) {
     bitblt(&display,menurect,offscreen,offscreen->rect.origin,F_STORE);
   }
   drawimenu(xicons,yicons,menurect);
   hitx = hity = -1;
   while (button3()) {
     jnap(1);
     ms = mouse.xy;
     ohitx = hitx; ohity = hity;
     if (ptinrect(ms,menurect)) {
       lasthitx = hitx = (ms.x-menurect.origin.x)/SPACING;
       lasthity = hity = (ms.y-menurect.origin.y)/SPACING;
     } else hitx = hity = -1;
     if (hitx == -1) {
	if (ohitx != -1) {
	    flipring(valley);
	    cursswitch(oldcursor);
	}
	continue;
     }
     if (ohitx == -1) {
	cursswitch(&white);
	valley = add(menurect.origin,Pt(SPACING*hitx,SPACING*hity));
	flipring(valley);
     } else if (ohitx != hitx || ohity != hity){
	flipring(valley);
	valley = add(menurect.origin,Pt(SPACING*hitx,SPACING*hity));
 	flipring(valley);
     }
   }
   if (offscreen != ((Bitmap *) 0)) {
     bitblt(offscreen,offscreen->rect,&display,menurect.origin,F_STORE);
     bfree(offscreen);
   }
   result.x = hitx;
   result.y = hity;
   cursswitch(oldcursor);
   return(result);
}

Rectangle canonrect(p1, p2)
Point p1, p2;
{
   Rectangle r;
   r.origin.x = min(p1.x, p2.x);
   r.origin.y = min(p1.y, p2.y);
   r.corner.x = max(p1.x, p2.x);
   r.corner.y = max(p1.y, p2.y);
   return(r);
}

int pttopt(p,q)
/* manhattan topology distance between two points */
Point p,q;
{
   return(abs(p.x-q.x)+abs(p.y-q.y));
}

Point nearestcorner(r,p)
Rectangle r;
Point p;
{
   int mindist,dist;
   Point minq,q;
   q = r.origin;
   mindist = pttopt(p,q); minq = q; 
   q.x = r.origin.x; q.y = r.corner.y;
   if ((dist = pttopt(p,q))<mindist) {mindist = dist; minq = q;}
   q = r.corner;
   if ((dist = pttopt(p,q))<mindist) {mindist = dist; minq = q;}
   q.x = r.corner.x; q.y = r.origin.y;
   if ((dist = pttopt(p,q))<mindist) {mindist = dist; minq = q;}
   return(minq);
}

outline(b,r)
Bitmap *b;
Rectangle  r;
{
   rectf(b,Rect(r.origin.x,r.origin.y,r.origin.x+1,r.corner.y),F_XOR);
   rectf(b,Rect(r.origin.x,r.corner.y,r.corner.x,r.corner.y+1),F_XOR);
   rectf(b,Rect(r.corner.x,r.origin.y+1,r.corner.x+1,r.corner.y+1),F_XOR);
   rectf(b,Rect(r.origin.x+1,r.origin.y,r.corner.x+1,r.origin.y+1),F_XOR);
}

Border(r)
Rectangle r;
{
  outline(&display,inset(r,1));
  outline(&display,inset(r,-1));
}

Rectangle sweeprect()
{
   Rectangle r;
   Point p1, p2;
   Cursor *oldcursor;
   oldcursor = cursswitch(&sweepcursor);
   while (!button123()) wait(MOUSE);
   p1=mouse.xy;
   p2=p1;
   r=canonrect(p1, p2);
   outline(&display,r);
   for(; button3(); nap(2)){
     outline(&display,r);
     p2=mouse.xy;
     r=canonrect(p1, p2);
     outline(&display,r);
   }
   outline(&display,r);
   cursswitch(oldcursor);
   return r;
}

bool getr(cliprect,sweep)
Rectangle cliprect;
Rectangle *sweep;
{
   int j;
   *sweep = sweeprect();
   return(rectclip(sweep,cliprect));
}

Point IconPoint(p)
/* convert screen coord to icon coord */
Point p;
{
  p.x = (p.x+Xsize-modx)/Xsize - divx - 1;
  p.y = (p.y+Ysize-mody)/Ysize - divy - 1;
  return(p);
}

short IconCoordX(x)
short x;
{
  return((x+Xsize-modx)/Xsize - divx - 1);
}

short IconCoordY(y)
short y;
{
  return((y+Ysize-mody)/Ysize - divy - 1);
}

Point ScreenPoint(p)
/* convert icon coord to screen coord */
Point p;
{
   p.x = p.x*Xsize + ICON.origin.x;
   p.y = p.y*Ysize + ICON.origin.y;
   return(p);
}

short ScreenCoordX(x)
short x;
{
  return(x*Xsize + ICON.origin.x);
}

short ScreenCoordY(y)
short y;
{
  return(y*Ysize + ICON.origin.y);
}


Rectangle IconRect(r)
/* convert a screen rectangle to the biggest totally contained icon rectangle */
Rectangle r;
{
   r.origin = IconPoint(add(r.origin,Pt(Xsize-1,Ysize-1)));
   r.corner = IconPoint(r.corner);
   if (horsize(r)<0) r.origin.x = r.corner.x;
   if (versize(r)<0) r.origin.y = r.corner.y;
   return(r);
}

Rectangle TrackBorder(r,buttonup)
/* tracks a rectangle "r" (in icon coords) and returns it as it is
   at the end of tracking, clipped to the icon */
Rectangle r;
bool buttonup;
{
   Point newP, oldP;
   Cursor *oldcursor;
   oldcursor = cursswitch(&white);
   outl.origin = nullpoint;
   outl.corner.x = horsize(r)*Xsize;
   outl.corner.y = versize(r)*Ysize;
   newP = ScreenPoint(r.origin);
   while (buttonup?(button123()):(!button123()))
      wait(MOUSE);
   cursset(newP);
   oldP = newP;
   Border(raddp(outl,newP));
   while (buttonup?(!button123()):(button123())) {
     wait(MOUSE);
      newP = ScreenPoint(IconPoint(mouse.xy));
     if (!eqpt(newP,oldP)) {
       Border(raddp(outl,oldP));
       Border(raddp(outl,newP));
       oldP = newP;
     }
   }
   Border(raddp(outl,newP));
   cursswitch(oldcursor);
   return(IconRect(raddp(outl,newP)));
}

bool SweepIconRect(r)
Rectangle *r;
{
   Point p;
   if (!getr(ICON,&sweep)) return(false);
   *r = IconRect(sweep);
   if ((horsize(*r)==0) || (versize(*r)==0)) return(false);
   return(true);
}

int GetIconRect(r)
/* returns: 0 if no rectangle is provided;
            2 if button2 is used (tracking a 16x16 rectangle);
            3 if button3 is used (sweeping a rectangle);
   if a rectangle is provided, r returns it in icon coordinates */       
Rectangle *r;
{
   int result;
   Cursor *oldcursor;
   oldcursor = cursswitch(&sweeportrack);
   while (!button123()) wait(MOUSE);
   if (button3()) {
     if (SweepIconRect(r)) result = 3;
     else result = 0;
   } else if (button2()) {
     *r = TrackBorder(raddp(rect16x16,add(IconPoint(mouse.xy),Pt(1,1))),false);
     result = 2;
   } else {while(button1())jnap(1); result = 0;}
   cursswitch(oldcursor);
   return(result);
}

bool GetIconPoint(p)
Point *p;
{
   Rectangle r;
   r = TrackBorder(raddp(Rect(0,0,1,1),IconPoint(mouse.xy)),true);
   while (button123())jnap(1);
   *p = r.origin;
   return((horsize(r) == 1) && (versize(r) == 1));
}

Bitmap *bittester;

bool bitmapbit(b,p)
Bitmap *b;
Point p;
{
   if (!(ptinrect(p,b->rect))) return(false);
   return (getpoint(b,p));
}

char getnibble(b,p)
Bitmap *b;
Point p;
{
   int nibble;
   nibble = 8*bitmapbit(b,p) + 4*bitmapbit(b,Pt(p.x+1,p.y)) +
            2*bitmapbit(b,Pt(p.x+2,p.y)) + bitmapbit(b,Pt(p.x+3,p.y));
   return(exa[nibble]);
}

putnibble(ch,b,clipr,p)
int ch;
Bitmap *b;
Rectangle clipr;
Point p;
{
   int nibble, mask;
   if (ch<'0') return;
   if (ch<='9') nibble = ch - '0';
   else if (ch<='F') nibble = 10 + (ch - 'A');
   else nibble = 10 + (ch - 'a');
   if (nibble != 0)
     for (mask = 0x10; mask >>= 1; p.x++)
       if ((mask & nibble) && ptinrect(p, clipr))
	  point(b, p, F_OR);
}

Bitmap *Source, *Undo;

FlipOntoScreen(p)
Point p;
{
   Rectangle ICONp;
   if (!ptinrect(p,Source->rect)) return(0);
   point(&display,add(icon.origin,p),F_XOR);
   ICONp.origin = add(Pt(p.x*Xsize,p.y*Ysize),ICON.origin);
   ICONp.corner = add(Pt((p.x+1)*Xsize,(p.y+1)*Ysize),ICON.origin);
   if ((ICONp.origin.x > icon.corner.x) ||
       (ICONp.origin.y > icon.corner.y))
    rectf(&display,ICONp,F_XOR);
}

bool geticonpoint(p)
Point p;
{
   if (!ptinrect(p,Source->rect)) return(false);
   return(bitmapbit(Source,p));
}

flipiconpoint(p)
Point p;
{
   point(Source,p,F_XOR);
   FlipOntoScreen(p);
}

IconOp(bit,p,op)
bool bit;
Point p;
int op;
{
   if ((p.x>=0) && (p.x<Xblocks) && (p.y>=0) && (p.y<Yblocks))
   switch (op) {
     case I_STORE:
       if (geticonpoint(p) != bit) flipiconpoint(p);
       break;
     case I_CLR:
       if (geticonpoint(p)) flipiconpoint(p);
       break;
     case I_OR:
       if (bit && !geticonpoint(p)) flipiconpoint(p);
       break;
     case I_XOR:
       if (bit) flipiconpoint(p);
       break;
     case I_AND:
       if (!bit && geticonpoint(p)) flipiconpoint(p);
       break;
   }
}

IconBitBlit(from,to,clip,srccode,dstcode)
Rectangle from;  /* icon coords */
Point to;        /* icon coords */
Rectangle clip;  /* icon coords */
int srccode,dstcode;
{
   Rectangle region;
   int dx,dy,di,dj,i,j;
   int left,up;
   bool bit;
   dx = to.x - from.origin.x;
   dy = to.y - from.origin.y;
   left = dx<0;
   up = dy<0;
   di = (left?1:-1);
   dj = (up?1:-1);
   region = from;
   if (!rectclip(&region,raddp(clip,sub(from.origin,to))))
     region = nullrect;
   for (j=(up?from.origin.y:from.corner.y-1);
        up?(j<from.corner.y):(j>=from.origin.y); j+=dj) {
     wait(CPU);
     for (i=(left?from.origin.x:from.corner.x-1);
          left?(i<from.corner.x):(i>=from.origin.x); i+=di) {
        bit = geticonpoint(Pt(i,j));
        IconOp(false,Pt(i,j),srccode);
        if (ptinrect(Pt(i,j),region)) IconOp(bit,Pt(i+dx,j+dy),dstcode);
     }
   }
}

horshear(b,r,dx,top)
Bitmap *b;
Rectangle r;
int dx;
bool top;
{
   int i,j,hsize,vsize,shift;
   bool bit,dir;
   hsize = horsize(r);
   vsize = versize(r);
   dir = (dx>0);
   for (j=0; j<vsize; j++) {
     wait(CPU);
     shift = top ? vsize-j-1 : j;
     bitblt(b,Rect(r.origin.x,r.origin.y+j,r.corner.x,r.origin.y+j+1),
             b,Pt(r.origin.x+muldiv(shift,dx,vsize),r.origin.y+j),
             F_STORE);
   }
}

vershear(b,r,dy,lft)
Bitmap *b;
Rectangle r;
int dy;
bool lft;
{
   int i,j,hsize,vsize,shift;
   bool bit,dir;
   hsize = horsize(r);
   vsize = versize(r);
   dir = (dy>0);
   for (i=0; i<hsize; i++) {
     wait(CPU);
     shift = lft ? hsize-i-1 : i;
     bitblt(b,Rect(r.origin.x+i,r.origin.y,r.origin.x+i+1,r.corner.y),
             b,Pt(r.origin.x+i,r.origin.y+muldiv(shift,dy,hsize)),
             F_STORE);
   }
}

OpRotPlus()
{
   int vsize,hsize,size;
   Rectangle r,rbuf;
   Bitmap *buffer;
   if (GetIconRect(&r)==0) return(0);
   hsize = horsize(r); vsize = versize(r); size = hsize+vsize;
   buffer = balloc(Rect(0,0,size,size));
   if (buffer == ((Bitmap *) 0)) return(0);
   rectf(buffer,buffer->rect,F_CLR);
   rbuf = rsubp(r,r.origin);
   SaveForUndo();
   cursswitch(&clock);
   bitblt(&display,raddp(r,icon.origin),buffer,rbuf.origin,F_XOR);
   horshear(buffer,rbuf,vsize,true);
   vershear(buffer,
            Rect(rbuf.origin.x,rbuf.origin.y,rbuf.corner.x+vsize,rbuf.corner.y),
            size,false);
   horshear(buffer,
            Rect(rbuf.origin.x,rbuf.corner.y-1,
                 rbuf.corner.x+vsize,rbuf.corner.y+hsize-1),
            -hsize,false);
   Erase(r);
   OrOntoPicture(buffer,
          Rect(rbuf.origin.x,rbuf.corner.y-1,
               rbuf.origin.x+vsize,rbuf.corner.y+hsize-1),
          add(r.origin,sub(Pt(hsize/2,vsize/2),Pt(vsize/2,hsize/2))));
   cursswitch((Cursor *) 0);
   bfree(buffer);
}

OpRotMinus()
{
   int vsize,hsize,size;
   Rectangle r,rbuf;
   Bitmap *buffer;
   if (GetIconRect(&r)==0) return(0);
   hsize = horsize(r); vsize = versize(r); size = hsize+vsize;
   buffer = balloc(Rect(0,0,size,size));
   if (buffer == ((Bitmap *) 0)) return(0);
   rectf(buffer,buffer->rect,F_CLR);
   rbuf = raddp(r,sub(Pt(vsize,0),r.origin));
   SaveForUndo();
   cursswitch(&clock);
   bitblt(&display,raddp(r,icon.origin),buffer,rbuf.origin,F_XOR);
   horshear(buffer,rbuf,-vsize,true);
   vershear(buffer,
            Rect(rbuf.origin.x-vsize,rbuf.origin.y,rbuf.corner.x,rbuf.corner.y),
            size,true);
   horshear(buffer,
            Rect(rbuf.origin.x-vsize,rbuf.corner.y-1,
                 rbuf.corner.x,rbuf.corner.y+hsize-1),
            hsize,false);
   Erase(r);
   OrOntoPicture(buffer,
          Rect(rbuf.corner.x-vsize,rbuf.corner.y-1,
               rbuf.corner.x,rbuf.corner.y+hsize-1),
          add(r.origin,sub(Pt(hsize/2,vsize/2),Pt(vsize/2,hsize/2))));
   cursswitch((Cursor *) 0);
   bfree(buffer);
}

OpReflY()
{
   Rectangle r;
   int i,j;
   bool bit1,bit2;
   if (GetIconRect(&r)==0) return(0);
   SaveForUndo();
   cursswitch(&clock);
   for (i=r.origin.x; i<r.corner.x; i+=1) {
     wait(CPU);
     for (j=0; j<(versize(r)/2); j+=1) {
        bit1 = geticonpoint(Pt(i,r.origin.y+j));
        bit2 = geticonpoint(Pt(i,r.corner.y-1-j));
        IconOp(bit1,Pt(i,r.corner.y-1-j),I_STORE);
        IconOp(bit2,Pt(i,r.origin.y+j),I_STORE);
     }
   }
   cursswitch((Cursor *) 0);
}

OpReflX()
{
   Rectangle r;
   int i,j;
   bool bit1,bit2;
   if (GetIconRect(&r)==0) return(0);
   SaveForUndo();
   cursswitch(&clock);
   for (i=0; i<(horsize(r)/2); i+=1) {
     wait(CPU);
     for (j=r.origin.y; j<r.corner.y; j+=1) {
        bit1 = geticonpoint(Pt(r.origin.x+i,j));
        bit2 = geticonpoint(Pt(r.corner.x-1-i,j));
        IconOp(bit1,Pt(r.corner.x-1-i,j),I_STORE);
        IconOp(bit2,Pt(r.origin.x+i,j),I_STORE);
     }
   }
   cursswitch((Cursor *) 0);
}

OpBlit(srcop,dstop)
int srcop,dstop;
{
   Rectangle r, r1;
   Point p;
   if (GetIconRect(&r)==0) return(0);
   r1 = TrackBorder(r,true);
   p = r1.origin;
   if (button23()) {
     while (button23())jnap(1);
     SaveForUndo();
     cursswitch(&clock);
     IconBitBlit(r,p,Rect(0,0,Xblocks,Yblocks),srcop,dstop);
     cursswitch((Cursor *) 0);
   } else while (button1())jnap(1);
}


OpGeneralBlit()
{
           int srcop, dstop;
           cursswitch(&menucursor);
           while (!button123()) wait(MOUSE);
           cursswitch((Cursor *) 0);
           if (!button3()) return(0);
           switch (menuhit(&blitsrcmenu,3)) {
             case NOHIT:
               srcop = I_NULL;
               break;
             case SRC_ID:
               srcop = I_OR;
               break;
             case SRC_CLR:
               srcop = I_CLR;
               break;
           }
           if (srcop==I_NULL) {cursswitch((Cursor *) 0); return(0);}
           cursswitch(&menucursor);
           while (!button123()) wait(MOUSE);
           cursswitch((Cursor *) 0);
           if (!button3()) return(0);
           switch (menuhit(&blitdstmenu,3)) {
             case NOHIT:
               dstop = I_NULL;
               break;
             case DST_STORE:
               dstop = I_STORE;
               break;
             case DST_OR:
               dstop = I_OR;
               break;
             case DST_XOR:
               dstop = I_XOR;
               break;
             case DST_AND:
               dstop = I_AND;
               break;
             case DST_CLR:
               dstop = I_CLR;
               break;
           }
           if (dstop==I_NULL) {cursswitch((Cursor *) 0); return(0);}
           OpBlit(srcop,dstop);
}

Erase(r)
Rectangle r;
{
   int i,j;
   for (j = r.origin.y ; j < r.corner.y ; j++) {
     wait(CPU);
     for (i = r.origin.x ; i < r.corner.x ; i++) {
       if (geticonpoint(Pt(i,j)) == true) flipiconpoint(Pt(i,j));
     }
   }
}

OpErase()
{
   Rectangle r;
   if (GetIconRect(&r)==0) return(0);
   SaveForUndo();
   cursswitch(&clock);
   Erase(r);
   cursswitch((Cursor *)0);
}

OpInvert()
{
   Rectangle r;
   int i,j;
   if (GetIconRect(&r)==0) return(0);
   SaveForUndo();
   cursswitch(&clock);
   for (j = r.origin.y ; j < r.corner.y ; j++) {
     wait(CPU);
     for (i = r.origin.x ; i < r.corner.x ; i++) {
       flipiconpoint(Pt(i,j));
     }
   }
   cursswitch((Cursor *)0);
}

getstr(s,p)
char *s;
Point p;
{
   char c,*t;
   static char str[]="x";
   t = s;
   for (;;) {
     wait(KBD);
     if (((c=kbdchar()) == '\r') || (c == '\n')) {
       *s = '\0';
       return;
     }
     if (c == '\b') {
       if (s>t) {
         str[0] = *(--s);
         string(&defont,str,&display,(p = sub(p,Pt(9,0))),F_XOR);
       }
     } else if ((c >= '!') && (c <= '~')) {
       if (s-t<50) {
         *s++ = (str[0] = c);
         p = string(&defont,str,&display,p,F_XOR);
       }
     }
   }
}

GetFNAME()
{
   Point p;
   Bitmap *b;
   cursswitch(&deadmouse);
   b = balloc(Rpt(Drect.origin,Pt(Drect.corner.x,Drect.origin.y+18)));
   if (b!=0) {
     bitblt(&display,b->rect,b,Drect.origin,F_STORE);
     rectf(&display,b->rect,F_CLR);
     outline(&display,inset(b->rect,1));
   }
   p = string(&defont,"File: ",&display,add(Drect.origin,Pt(10,3)),F_XOR);
   getstr(FNAME,p);
   p = string(&defont,"File: ",&display,add(Drect.origin,Pt(10,3)),F_XOR);
   string(&defont,FNAME,&display,p,F_XOR);
   if (b!=0) {
     bitblt(b,b->rect,&display,Drect.origin,F_STORE);
     bfree(b);
   }
   cursswitch((Cursor *) 0);
}

Bitmap *pickupmap;

PickUpCursor()
{
/*
   Cursor *oldcursor;
   Cursor pickuptexture;
   Rectangle r;
   int i;
   r = TrackBorder(raddp(rect16x16,IconPoint(mouse.xy)),true);
   r = raddp(r,icon.origin);
   bitblt(&display,r,pickupmap,Pt(0,0),F_STORE);
   for (i=0; i<16; i++) pickuptexture.bits[i] = (short)(pickupmap->base[i]>>16);
   oldcursor = cursswitch(&pickuptexture);
   while (button123())jnap(1);
   while (!button123())jnap(1);
   while (button123())jnap(1);
   cursswitch(oldcursor);
*/
}

char SFbuffer[100];
char *SFlist[] = {		/* Where to look */
    "",				/* Look in local directory first */
    "/usr/jerq/icon/16x16/",	/* Ick! "/usr/jerq/icon" should be a parameter */
    "/usr/jerq/icon/texture/",
    "/usr/jerq/icon/large/",
    "/usr/jerq/icon/face48/",
    (char *) 0
};

FILE *SearchFile(filename,mode,filefound)
char *filename, *mode, **filefound;
{
   FILE *fp;
   char **sf = SFlist;
   int namez = strlen(filename);
   *filefound = SFbuffer;
   fp = (FILE *) 0;
   for (; (fp == (FILE *) 0) && (*sf != (char *) 0); sf++) {
     if ((strlen(*sf) + namez) < sizeof(SFbuffer)) {
       strcpy(SFbuffer, *sf);
       strcat(SFbuffer, filename);
       fp = fopen(SFbuffer, mode);
     }
   }
   return(fp);
}

Rectangle OpLoad(bitmap,filename)
Bitmap *bitmap;
char *filename;
{
   FILE *fp;
   Rectangle rect;
   Cursor *oldcursor;
   char *filefound;
   int ch,i,j;
   int xsize,ysize;
   rect = bitmap->rect;
   oldcursor = cursswitch(&clock);
   fp = SearchFile(filename,"r",&filefound);
   if (fp == ((FILE *) 0)) {cursswitch((Cursor *) 0); return(nullrect);}
   ch = getc(fp);
   if (ch=='0') {
     i = rect.origin.x; j = rect.origin.y;
     xsize = 0;
     for (;;) {
       getc(fp);			/* 'x' */
       putnibble(getc(fp),bitmap,rect,Pt(i,j)); i+=4;
       putnibble(getc(fp),bitmap,rect,Pt(i,j)); i+=4;
       putnibble(getc(fp),bitmap,rect,Pt(i,j)); i+=4;
       putnibble(getc(fp),bitmap,rect,Pt(i,j)); i+=4;
       getc(fp); /* ',' */
       while ((ch = getc(fp))==' '); 	/* '0' or '\n' */
       if (ch=='\n') {
         xsize = max(xsize,i-rect.origin.x);
         i = rect.origin.x;
         j++;
         ch = getc(fp);  	/* '0' */
       } else if (ch == EOF) break;
     }
    ysize = j-rect.origin.y;
   } else {
     while ((ch!='{')&&(ch!=EOF)) ch=getc(fp);
     for (j=rect.origin.y; j<rect.origin.y+16; j++) {
       while (((ch=getc(fp))!='x')&&(ch!='X')&&(ch!=EOF)) {};
       putnibble(getc(fp),bitmap,rect,Pt(rect.origin.x,j));
       putnibble(getc(fp),bitmap,rect,Pt(rect.origin.x+4,j));
       putnibble(getc(fp),bitmap,rect,Pt(rect.origin.x+8,j));
       putnibble(getc(fp),bitmap,rect,Pt(rect.origin.x+12,j));
     }
     xsize = ysize = 16;
   }
   fclose(fp);
   rect.origin.x = 0; rect.origin.y = 0; 
   rect.corner.x = xsize; rect.corner.y = ysize;
   cursswitch(oldcursor);
   return(rect);
}

OpRead()
{
   Rectangle rect, r1;
   Bitmap *buffer;
   Point p;
   GetFNAME();
   if (!FNAME[0]) return(0);
   buffer = balloc(Source->rect);
   if (buffer == ((Bitmap *) 0)) {cursswitch((Cursor *) 0); return(0);}
   rectf(buffer,buffer->rect,F_CLR);
   rect = OpLoad(buffer,FNAME);
   if (!eqrect(rect,nullrect)) {
     SaveForUndo();
     r1 = TrackBorder(raddp(rect,IconPoint(mouse.xy)),true);
     p = r1.origin;
     while (button3())jnap(1);
     cursswitch(&clock);
     OrOntoPicture(buffer,rect,p);
     cursswitch((Cursor *)0);
   }
   bfree(buffer);
}

#ifdef BSD
#define BUFSIZE BUFSIZ
#endif BSD
char buffer[BUFSIZE];
char *bufend;

bclear()
{
	bufend = buffer;
}

bsend(fp)
FILE *fp;
{
	*bufend = '\0';
	fputs(buffer,fp);
	bclear();
}

bputc(c,fp)
char c;
FILE *fp;
{
   if (bufend >= buffer+BUFSIZE-3) bsend(fp);
   *bufend++ = c;
}

OpWrite()
{
   FILE *fp;
   Rectangle r;
   Point p;
   int i,j,butt;
   butt = GetIconRect(&r);
   if (butt==0) return(0);
   if (butt==3) { 
     GetFNAME();
     if (!FNAME[0]) return(0);
     fp = fopen(FNAME,"w");
     if (fp == ((FILE *) 0)) return(0);
     rectf(&display,Drect,F_XOR);
     bclear();
     for (j = r.origin.y; j<r.corner.y; j++) {
       for (i=r.origin.x; i<r.corner.x; i+=16) {
         bputc('0',fp);bputc('x',fp);
         bputc(getnibble(Source,Pt(i,j)),fp);
	 bputc(getnibble(Source,Pt(i+4,j)),fp);
	 bputc(getnibble(Source,Pt(i+8,j)),fp);
	 bputc(getnibble(Source,Pt(i+12,j)),fp);
         bputc(',',fp);
       }
       bputc('\n',fp);
       bsend(fp);
     }
     fclose(fp);
     rectf(&display,Drect,F_XOR);
   } else if (butt==2) {
     GetFNAME();
     if (!FNAME[0]) return(0);
     fp = fopen(FNAME,"w");
     if (fp == ((FILE *) 0)) return(0);
     rectf(&display,Drect,F_XOR);
     fputs("Texture ",fp); fputs(FNAME,fp); fputs(" = {\n",fp);
     j = r.origin.y; i = r.origin.x;
     bclear();
     while (j < r.corner.y) {
       if (((j-r.origin.y)%4) == 0) bputc('\t',fp);
       bputc(' ',fp);bputc('0',fp);bputc('x',fp);
       bputc(getnibble(Source,Pt(i,j)),fp);
       bputc(getnibble(Source,Pt(i+4,j)),fp);
       bputc(getnibble(Source,Pt(i+8,j)),fp);
       bputc(getnibble(Source,Pt(i+12,j)),fp);
       bputc(',',fp);
       if (((j-r.origin.y)%4) == 3) bputc('\n',fp);
       j = j+1;
       bsend(fp);
     }
     fputs("};\n",fp);
     fclose(fp);
     rectf(&display,Drect,F_XOR);
   }

}

OpTexture()
{
   Bitmap *buffer,*buffer1;
   Rectangle source,dest;
   Point target;
   int repx,repy,i,j,hsize,vsize;
   if (GetIconRect(&source)==0) return(0);
   if (GetIconRect(&dest)==0) return(0);
   hsize = horsize(source);
   vsize = versize(source);
   if ((hsize==0) || (vsize==0)) return(0);
   buffer = balloc(source);
   if (buffer == ((Bitmap *) 0)) return(0);
   buffer1 = balloc(source);
   if (buffer1 == ((Bitmap *) 0)) {bfree(buffer); return(0);}
   SaveForUndo();
   cursswitch(&clock);
   bitblt(&display,raddp(source,icon.origin),buffer,buffer->rect.origin,F_STORE);
   repx = horsize(dest)/hsize;
   repy = versize(dest)/vsize;
   for (j=0; j<=repy; j++)
     for (i=0; i<=repx; i++) {
       bitblt(buffer,buffer->rect,buffer1,buffer1->rect.origin,F_STORE);
       OrOntoPictureClipped(buffer1,
         buffer1->rect,add(dest.origin,Pt(i*hsize,j*vsize)),dest);
     }
   cursswitch((Cursor *) 0);
   bfree(buffer);
   bfree(buffer1);
}

int GridSwitch=0;

OpGrid()
{
   if ((GridSwitch%2)==0) DrawFineGrid(); else DrawCoarseGrid();
   GridSwitch = (GridSwitch+1)%4;
}

OpResize()
{
   Point p;
   Rectangle r;
   Bitmap *NewSource, *NewUndo;
   Cursor *oldcursor;
   bool doit;
   oldcursor = cursswitch(&clock);
   p=mouse.xy;
   r=canonrect(icon.origin,p);
   outline(&display,r);
   for(; !button123(); nap(2)){
     outline(&display,r);
     p=mouse.xy;
     r=canonrect(icon.origin,p);
     outline(&display,r);
   }
   doit = button3();
   outline(&display,r);
   cursswitch(oldcursor);
   while (button123())jnap(1);
   if (!doit) return(0);
   if (!rectclip(&r,Rpt(icon.origin,ICON.corner))) return(0);
   r = rsubp(r,icon.origin);
   NewSource = balloc(r); if (NewSource==0) return(0);
   NewUndo = balloc(r); if (NewUndo==0) {bfree(NewSource); return(0);}
   rectf(NewSource,NewSource->rect,F_CLR);
   rectf(NewUndo,NewUndo->rect,F_CLR);
   bitblt(Source,Source->rect,NewSource,NewSource->rect.origin,F_XOR);
   bitblt(Undo,Undo->rect,NewUndo,NewUndo->rect.origin,F_XOR);
   bfree(Source); bfree(Undo);
   Source = NewSource; Undo = NewUndo;
   rectf(&display,Drect,F_CLR);
   XSIZE = horsize(r); YSIZE = versize(r);
   Redraw();
}


HorShear(r,dx,top)
Rectangle r;
int dx;
bool top;
{
   int j,vsize,shift;
   vsize = versize(r);
   for (j=0; j<vsize; j++) {
     shift = top ? vsize-j-1 : j;
     IconBitBlit(Rect(r.origin.x,r.origin.y+j,r.corner.x,r.origin.y+j+1),
                 Pt(r.origin.x+muldiv(shift,dx,vsize),r.origin.y+j),
                 Rect(0,0,Xblocks,Yblocks),
                 I_CLR,I_OR);
   }
}

VerShear(r,dy,lft)
Rectangle r;
int dy;
bool lft;
{
   int i,hsize,shift;
   hsize = horsize(r);
   for (i=0; i<hsize; i++) {
     shift = lft ? hsize-i-1 : i;
     IconBitBlit(Rect(r.origin.x+i,r.origin.y,r.origin.x+i+1,r.corner.y),
                 Pt(r.origin.x+i,r.origin.y+muldiv(shift,dy,hsize)),
                 Rect(0,0,Xblocks,Yblocks),
                 I_CLR,I_OR);
   }
}

OpHorShear()
{
   Rectangle r;
   int dx;
   bool top;
   Point p,nearcorner;
   if (GetIconRect(&r)==0) return(0);
   if ((horsize(r)==0) || (versize(r)==0)) return(0);
   if (!GetIconPoint(&p)) return(0);
   SaveForUndo();
   cursswitch(&clock);
   nearcorner = nearestcorner(r,p);
   dx = p.x - nearcorner.x;
   top = (nearcorner.y == r.origin.y);
   HorShear(r,dx,top);
   cursswitch((Cursor *) 0);
}

OpVerShear()
{
   Rectangle r;
   int dy;
   bool lft;
   Point p,nearcorner;
   if (GetIconRect(&r)==0) return(0);
   if ((horsize(r)==0) || (versize(r)==0)) return(0);
   if (!GetIconPoint(&p)) return(0);
   SaveForUndo();
   cursswitch(&clock);
   nearcorner = nearestcorner(r,p);
   dy = p.y - nearcorner.y;
   lft = (nearcorner.x == r.origin.x);
   VerShear(r,dy,lft);
   cursswitch((Cursor *) 0);
}

Stretch(sb,sr,db,dr,op)
Bitmap *sb,*db;
Rectangle sr,dr;
Code op;
{
   int i,j,shsize,svsize,dhsize,dvsize;
   shsize = horsize(sr);
   svsize = versize(sr);
   dhsize = horsize(dr);
   dvsize = versize(dr);
   for (j=0; j<svsize; j++) {
     wait(CPU);
     for (i=0; i<shsize; i++) {
       bitblt(sb,
              Rect(sr.origin.x+i,sr.origin.y+j,sr.origin.x+i+1,sr.origin.y+j+1),
              db,
              Pt(dr.origin.x+muldiv(dhsize,i,shsize),
                 dr.origin.y+muldiv(dvsize,j,svsize)),
              op);
     }
   }
}

OpStretch()
{
   Bitmap *buffer;
   Rectangle source,dest;
   if (GetIconRect(&source)==0) return(0);
   if ((horsize(source)==0) || (versize(source)==0)) return(0);
   if (GetIconRect(&dest)==0) return(0);
   if ((horsize(dest)==0) || (versize(dest)==0)) return(0);
   buffer = balloc(dest);
   if (buffer == ((Bitmap *) 0)) return(0);
   SaveForUndo();
   cursswitch(&clock);
   rectf(buffer,buffer->rect,F_CLR);
   Stretch(&display,raddp(source,icon.origin),buffer,buffer->rect,F_XOR);
   IconBitBlit(source,source.origin,Rect(0,0,0,0),F_CLR,F_CLR);
   OrOntoPictureClipped(buffer,buffer->rect,dest.origin,dest);
   cursswitch((Cursor *) 0);
   bfree(buffer);
}

DrawHorGridLine(p,op)
Point p;
Code op;
{
   if ((p.y < icon.corner.y+2) && (p.x < icon.corner.x+2))
     p.x = ScreenCoordX(IconCoordX(icon.corner.x+Xsize));
   rectf(&display,Rpt(p,Pt(ICON.corner.x,p.y+1)),op);
}

DrawVerGridLine(p,op)
Point p;
Code op;
{
   if ((p.x < icon.corner.x+2) && (p.y < icon.corner.y+2))
     p.y = ScreenCoordY(IconCoordY(icon.corner.y+Ysize));
   rectf(&display,Rpt(p,Pt(p.x+1,ICON.corner.y)),op);
}

DrawGridBorder()
{
   DrawHorGridLine(ICON.origin,F_OR);
   DrawHorGridLine(add(ICON.origin,Pt(0,Yblocks*Ysize)),F_OR);
   DrawVerGridLine(ICON.origin,F_OR);
   DrawVerGridLine(add(ICON.origin,Pt(Xblocks*Xsize,0)),F_OR);
   point(&display,ICON.corner,F_OR);
}

DrawFineGrid()
{
   register int i,j;
   for (j=1; j<Yblocks; j++) DrawHorGridLine(add(ICON.origin,Pt(0,j*Ysize)),F_XOR);
   for (i=1; i<Xblocks; i++) DrawVerGridLine(add(ICON.origin,Pt(i*Xsize,0)),F_XOR);
}

DrawCoarseGrid()
{
   Point p;
   register int i,j;
   for (j=0; j<=Yblocks; j+=16) 
     DrawHorGridLine(add(ICON.origin,Pt(0,j*Ysize+1)),F_XOR);
   for (j=8; j<=Yblocks; j+=16) {
     p=add(ICON.origin,Pt(0,j*Ysize+1));
     if ((p.y < icon.corner.y+2) && (p.x < icon.corner.x+2))
       p.x = ScreenCoordX(IconCoordX(icon.corner.x+Xsize));
     for (i=0; i<=Xblocks*Xsize; i+=2)
       point(&display,Pt(p.x+i,p.y),F_XOR);
   }
   for (i=0; i<=Xblocks; i+=16) 
     DrawVerGridLine(add(ICON.origin,Pt(i*Xsize+1,0)),F_XOR);
   for (i=8; i<=Xblocks; i+=16) {
     p=add(ICON.origin,Pt(i*Xsize+1,0));
     if ((p.x < icon.corner.x+2) && (p.y < icon.corner.y+2))
       p.y = ScreenCoordY(IconCoordY(icon.corner.y+Ysize));
     for (j=0; j<=Yblocks*Ysize; j+=2)
       point(&display,Pt(p.x,p.y+j),F_XOR);
   }
}

Bitmap *HelpBuffer, *HelpTexture;

helpline(i,icon,str)
int i;
Texture *icon;
char *str;
{
   texture(HelpTexture,HelpTexture->rect,icon,F_STORE);
   bitblt(HelpTexture,HelpTexture->rect,HelpBuffer,Pt(2,2+16*i),F_XOR);
   string(&defont,str,HelpBuffer,Pt(2+32,2+16*i),F_XOR);
}

HelpSorry()
{
     string(&defont,"Not enough space on blit",
            &display,add(icon.corner,Pt(15,-20)),F_XOR);
}

Help()
{
   HelpBuffer = balloc(Rect(0,0,302,482));
   HelpTexture = balloc(Rect(0,0,16,16));
   if ((HelpBuffer==0) || (HelpTexture==0)) {
     HelpSorry();
     while (!button123()) wait(MOUSE);
     HelpSorry();
     while (button123())jnap(1); 
     return(0);
   }
   rectf(HelpBuffer,HelpBuffer->rect,F_CLR);
   outline(HelpBuffer,Rpt(HelpBuffer->rect.origin,sub(HelpBuffer->rect.corner,Pt(1,1))));
   helpline(0,&whiteT,"left button: draw");
   helpline(1,&whiteT,"middle button: erase");
   helpline(2,&whiteT,"");
   helpline(3,&Cmove,"move region");
   helpline(4,&Ccopy,"copy region");
   helpline(5,&Cinvert,"invert region");
   helpline(6,&Cerase,"erase region");
   helpline(7,&Creflx,"reflect x region");
   helpline(8,&Crefly,"reflect y region");
   helpline(9,&Crotplus,"rotate + region");
   helpline(10,&Crotminus,"rotate - region");
   helpline(11,&Cshearx,"shear x region");
   helpline(12,&Csheary,"shear y region");
   helpline(13,&Cstretch,"stretch region");
   helpline(14,&Ctexture,"texture region");
   helpline(15,&Cread,"read file");
   helpline(16,&Cgrid,"background grids");
   helpline(17,&Cresize,"resize drawing area");
   helpline(18,&Cwrite,"write file");
   helpline(19,&Cblit,"bitblit region");
   helpline(20,&Ccursor,"pick cursor icon");
   helpline(21,&Chelp,"(press a button to continue)");
   helpline(22,&Cundo,"undo last operation");
   helpline(23,&whiteT,"");
   helpline(24,&clockT,"wait");
   helpline(25,&deadmouseT,"mouse inactive");
   helpline(26,&menucursorT,  "menu on right button");
   helpline(27,&sweepcursorT,"sweep rect (right button)");
   helpline(28,&sweeportrackT,"sweep rect (right button) or");
   helpline(29,&whiteT,"get 16x16 frame (middle butt)");
   screenswap(HelpBuffer,HelpBuffer->rect,raddp(HelpBuffer->rect,Drect.origin));
   while (!button123())jnap(1);
   screenswap(HelpBuffer,HelpBuffer->rect,raddp(HelpBuffer->rect,Drect.origin));
   while (button123())jnap(1);
   bfree(HelpBuffer);
   bfree(HelpTexture);
}

SaveForUndo()
{
   bitblt(Source,Source->rect,Undo,Undo->rect.origin,F_STORE);
}

OpUndo()
{
   Rectangle r;
   int hsize,vsize,i,j;
   bool bit;
   Cursor *oldcursor;
   oldcursor = cursswitch(&clock);
   r = Source->rect;
   bitblt(Undo,r,Source,r.origin,F_XOR);
   XorOntoScreen(Source,r,r.origin);
   bitblt(Source,r,Undo,r.origin,F_XOR);
   bitblt(Undo,r,Source,r.origin,F_XOR);
   cursswitch(oldcursor);
}

XorOntoScreen(b,r,p)
Bitmap *b;
Rectangle r;
Point p;
{
   int i,j,h,v;
   h = horsize(r); v = versize(r);
   for(j=0; j<h; j++) {
     wait(CPU);
     for (i=0; i<v; i++) {
       if (bitmapbit(b,add(r.origin,Pt(j,i)))) 
         FlipOntoScreen(add(p,Pt(j,i)));
     }
   }
}

XorOntoPicture(b,r,p)
Bitmap *b;
Rectangle r;
Point p;
{
   XorOntoScreen(b,r,p);
   bitblt(b,r,Source,p,F_XOR);
}

OrOntoPicture(b,r,p)
Bitmap *b;
Rectangle r;
Point p;
/* Scrambles the contents of b */
{
   rectf(b,r,F_XOR);
   bitblt(Source,raddp(rsubp(r,r.origin),p),b,r.origin,F_OR);
   rectf(b,r,F_XOR);
   XorOntoPicture(b,r,p);
}

OrOntoPictureClipped(b,r,p,clip)
Bitmap *b;
Rectangle r;
Point p;
Rectangle clip;
{
   if (!rectclip(&r,rsubp(clip,sub(p,r.origin)))) r = nullrect;
   OrOntoPicture(b,r,p);
}

bool FirstTime = true;

Redraw()
{
   Xblocks = XSIZE;
   Yblocks = YSIZE;

   Xsize = (horsize(Drect)-1)/Xblocks;
   Ysize = (versize(Drect)-1)/Yblocks;

   if (Xsize==0) Xsize = 1;
   if (Ysize==0) Ysize = 1;

   Ysize = (Xsize = (Xsize<Ysize)?Xsize:Ysize);

   icon.origin = add(Drect.origin,Pt(2,2));
   icon.corner = add(icon.origin,Pt(Xblocks,Yblocks));

   ICON.origin = sub(Drect.corner,Pt(1+Xsize*Xblocks,1+Ysize*Yblocks));
   ICON.corner = add(ICON.origin,Pt(Xsize*Xblocks,Ysize*Yblocks));
   modx = ICON.origin.x % Xsize;
   divx = ICON.origin.x / Xsize;
   mody = ICON.origin.y % Ysize;
   divy = ICON.origin.y / Ysize;

   rectf(&display, Drect, F_CLR);
   DrawGridBorder();
   outline(&display,inset(Rpt(icon.origin,add(icon.corner,Pt(-1,-1))),-2));
   if ((GridSwitch==1)||(GridSwitch==2)) DrawFineGrid();
   if ((GridSwitch==2)||(GridSwitch==3)) DrawCoarseGrid();

   if (FirstTime) FirstTime = false; 
   else {
     cursswitch(&clock);
     XorOntoScreen(Source,Source->rect,Source->rect.origin);
     cursswitch((Cursor *)0);
   }
}

Icon()
{
   Bitmap *b;
   Point p,cur,hit;
   int i,j;

   Redraw();

   cur.x = 0; cur.y = 0;

   for (;;) {
     wait(MOUSE);
     if (P->state&RESHAPED) return(0);
     if (button1()) {
       SaveForUndo();
       while(button1()) {
         if (ptinrect((p=mouse.xy),ICON)) {
           p = sub(p,ICON.origin);
           cur.x = p.x/Xsize;
           cur.y = p.y/Ysize;
           if (geticonpoint(cur) == false) flipiconpoint(cur);
         }
         wait(MOUSE);
       }
     } else if (button2()) {
       SaveForUndo();
       while(button2()) {
         if (ptinrect((p=mouse.xy),ICON)) {
           p = sub(p,ICON.origin);
           cur.x = p.x/Xsize;
           cur.y = p.y/Ysize;
           if (geticonpoint(cur) == true) flipiconpoint(cur);
         }
         wait(MOUSE);
       }
     }
     if (button3()) {
       hit = imenuhit(imenu);
       switch (5*hit.y+hit.x) {
         case BLIT: OpGeneralBlit(); break;
         case MOVE: OpBlit(I_CLR,I_OR); break;
         case COPY: OpBlit(I_OR,I_OR); break;
         case ERASE: OpErase(); break;
         case INVERT: OpInvert(); break;
         case REFLECTX: OpReflX(); break;
         case REFLECTY: OpReflY(); break;
         case ROTATEPLUS: OpRotPlus(); break;
         case ROTATEMINUS: OpRotMinus(); break;
         case SHEARX: OpHorShear(); break;
         case SHEARY: OpVerShear(); break;
         case STRETCH: OpStretch(); break;
         case TEXTURE: OpTexture(); break;
         case GRID: OpGrid(); break;
         case PICK: PickUpCursor(); break;
         case READ: OpRead(); break;
         case WRITE: OpWrite(); break;
         case RESIZE: OpResize(); break;
         case HELP: Help(); break;
         case UNDO: OpUndo(); break;
       }
     }
   }
}


main(argc,argv)
char *argv[];
{
  request(MOUSE|SEND|KBD);
  mousemotion();
  initdisplay(argc, argv);
  initicons();

  nullpoint.x = 0;
  nullpoint.y = 0;
  point16x16.x = 16;
  point16x16.y = 16;
  nullrect.origin = nullpoint;
  nullrect.corner = nullpoint;
  rect16x16.origin = nullpoint;
  rect16x16.corner = point16x16;

  bittester = balloc(Rect(0,0,1,1));
  pickupmap = balloc(Rect(0,0,16,16));
  Source = balloc(Rect(0,0,XSIZE,YSIZE));
  if (Source == ((Bitmap *) 0)) exit();
  rectf(Source,Source->rect,F_CLR);
  Undo = balloc(Rect(0,0,XSIZE,YSIZE));
  if (Undo == ((Bitmap *) 0)) exit();
  rectf(Undo,Undo->rect,F_CLR);

  do {
    P->state&=~RESHAPED;
    Icon();
  } while (P->state&RESHAPED);

  exit();

}
