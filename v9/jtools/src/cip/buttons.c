/*
  %Z%  %M%  version %I% %Q%of %H% %T%
  Last Delta:  %G% %U% to %P%
*/

#include "cip.h"

extern void getKbdText ();
extern int currentBrush, copyFlag, thingSelected, editDepth;
extern Rectangle brushes[];
extern int gridState, videoState, buttonState;

struct thing *
doMouseButtons(t,offset) 
register struct thing *t;
Point offset;
{
  Point m;
  register Rectangle *b;
  register struct thing *t1;
 
    if (P->state & RESHAPED) {
      /* Redraw screen if layer has been reshaped */
      redrawLayer (t, offset);
      return(t);
    }
    if (ptinrect (MOUSE_XY, brushes[PIC])) {
      if (P->cursor != &crossHairs) {
	cursswitch (&crossHairs);
      }
    }
    else {
      if (P->cursor == &crossHairs) {
	cursswitch ((Texture *)0);
      }
    }
    m = sub(MOUSE_XY,offset);
    if (button1()) {
      b = Select(t, add(m,offset), offset);
      if ( b == &brushes[PIC]) {
	copyFlag=0;
	t1 = selectThing(m,t);
	if (thingSelected==1) {
	  drawSelectionLines(t,offset);
	}
	if (t1 != TNULL) {
	  t = t1;
	  thingSelected = 1;
	  changeBrush(-1);
	  if (t->type==MACRO||t->type==TEXT) {
	    changeButtons(MOVEbuttons);
	  }
	  else {
	    changeButtons(EDITbuttons);
	  }
	  for(flashThing(t,offset);button1();jnap(2)) ;
	  drawSelectionLines(t,offset);
	  flashThing(t,offset);
	}
	else {
	  thingSelected = 0;
	  changeButtons((currentBrush>=0)? DRAWbuttons : INITbuttons);
	}
      }
      else {
	if (thingSelected==1) {
	  drawSelectionLines(t,offset);
	  thingSelected = 0;
	}
	if (b != (Rectangle *) NULL ) {
	  for (; button1(); jnap(2)) ;
	  copyFlag=0;
	  thingSelected = 0;
	  changeButtons(DRAWbuttons);
	}
      }
      for (; button1(); jnap(2)) ;
    }
    else {
      if (button2()) {
	if (thingSelected) { 
	  if (copyFlag!=0) {
	    drawSelectionLines (t, offset);
	    t = insert(copyThing(t,m,offset),t);
	    drawSelectionLines (t, offset);
	    /* clear button 2 - only one copy per click */
	    for ( ; button2(); jnap(2)) ;
	  }
	  else {
	    editThing(m,offset,t);
	  }
	}
	else {
	  if ((currentBrush>=0)&&(ptinrect(add(m,offset),brushes[PIC]))) {
	    if (currentBrush==SPLINE) changeButtons(SPLINEbuttons);
	    t = place(currentBrush,m,t,offset);
	    changeButtons(DRAWbuttons);
	  }
	  else {
	    for ( ; button2(); jnap(2)) ;
	  }
	}
      }
      else {
	if (button3()) {
	  if (thingSelected) {
	    t = displayThingMenu(m,t,offset);
	  }
	  else {
	    t = displayCommandMenu(t, offset);
	  }
	}
      }
    }	
  return(t);
}

struct thing *
place(b,p,h,os)
register int b;
Point p, os;
struct thing *h;
{
  register struct thing *t;
  Point r;
  register Point *plist, *olist;
  register int i, used, room;
  struct thing dummy;
  register char *s, c;

  r = near(add(p,os),h,os);
  if (r.x!=0) {
    p = sub(r,os);
  }
  switch (b) {
    case CIRCLE: {
      if ((t = newCircle(p)) != TNULL) {
	t->otherValues.radius = 1;
	draw(t,os);
	h = insert(t,h);
	track(p,os,GROWCIRCLE,t);
	BoundingBox(t);
      }
      break;	
    }
    case BOX: {
      h = insert(&dummy,h);
      r = track(p,os,BOX,h);
      if ((t = newBox(canon (p, r))) != TNULL) {
	h = remove (&dummy);
	h = insert(t,h);
      }
      else {
	draw (h, os);
	h = remove (&dummy);
      }
      break;	
    }
    case ELLIPSE: {
      if ((t = newEllipse(p)) != TNULL) {
	t->otherValues.ellipse.ht = 1;
	t->otherValues.ellipse.wid = 1;
	draw(t,os);
	h = insert(t,h);
	track(p,os,ELLIPSE,t);
	BoundingBox(t);
      }
      break;	
    }
    case LINE: {
      h = insert(&dummy,h);
      r = track(p,os,LINE,h);
      if ((t = newLine(p,r)) != TNULL) {
	h = remove (&dummy);
	h = insert(t,h);
      }
      else {
	draw (h, os);
	h = remove (&dummy);
      }
      break;	
    }
    case ARC: {
      h = insert(&dummy,h);
      r = track(p,os,ARC,h);
      if ((t = newArc(p,r)) != TNULL) {
	h = remove (&dummy);
	h = insert(t,h);
      }
      else {
	draw (h, os);
	h = remove (&dummy);
      }
      break;	
    }
    case SPLINE: {
      if ((plist = (Point *)getSpace(5*sizeof(Point)))!=(Point *)NULL) {
	h = insert(&dummy,h);
	plist[1]=p;
	used = 1; room = 3;
	do {
	  if (used==room) {
	    olist = plist;
	    room <<= 1;
	    plist = (Point *) getSpace((room+2)*sizeof(Point));
	    if (plist==(Point *)NULL) {
	      draw (&dummy, os);
	      h = remove (&dummy);
	      plist = olist;		/* Free list later */
	      used = 0;
	      break;
	    }
	    for (i=1; i<=used; i++) {
	      plist[i]=olist[i];
	    }
	    free(olist);
	  }
	  if (button2()) {
	    ++used;
	    plist[used]= track(plist[used-1],os,LINE,h);
	    xsegment(add(os,plist[used-1]),
	              add(os,plist[used]));
	  }
	  jnap(2);
	} while (!button3());
	for (; button3(); jnap(2)) ;
	drawZigZag(os,plist,used);
	if (used>2) {
	  t = newSpline(++used,room,plist);
	}
	else {
	  t = TNULL;
	  free (plist);
	}
	h = remove(&dummy);
	h = insert(t,h);
      }
      break;
    }
    case TEXT: {
      register char s[MAXTEXT+1];
      register char *ss;
      register char *q, *qq;
      getKbdText (s, add(p,os), 0, &carrot, MAXTEXT);
      ss = s;
      t = TNULL;
      if ((q=getSpace(strlen(s)))!=NULL) {
	for (qq=q; *qq++ = *ss++;) {
	}
	t = newText(p,q);
	h = insert(t,h);
      }
      break;
    }
  }
  if (t!=TNULL) {
    if (t->type != CIRCLE && t->type != ELLIPSE) {
      draw(t,os);
    }
  }
  return(h);
}

removeReflectionReferences(m)
register struct macro *m;
{
  register struct macro *t;

  if ((t=m)!=(struct macro *)NULL) {
    t->xReflectionOf = (struct macro *) NULL;
    t->yReflectionOf = (struct macro *) NULL;
    for (t=t->next; t!=(struct macro *)NULL; t=t->next) {
      if (t->xReflectionOf==m) {
        t->xReflectionOf = (struct macro *) NULL;
      }
      if (t->yReflectionOf==m) {
        t->yReflectionOf = (struct macro *) NULL;
      }
    } 
  }
}

clearKeyboard()
{
  for (; kbdchar() != -1; ) ;
}

redrawLayer (h, offset)
struct thing *h;
Point offset;
{
  cursinhibit();
  brushInit();	/* Init outline of brush boxes. */
  drawFrame(); 	/* display drawing area and brushes */
  currentBrush = -1;
  buttonState = INITbuttons;
  thingSelected = 0;
  copyFlag = 0;
  doRedraw (h, offset);
  if (videoState == BLACKfield) {
    rectf(&display, Drect,F_XOR);
  }
  if (editDepth) {
    drawEDbutton (editDepth);
  }
  cursallow();
  P->state &= ~RESHAPED;
}
