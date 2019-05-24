#include "copyright.h"

/* $Header: XKeyBind.c,v 11.35 87/09/08 20:17:55 toddb Exp $ */
/* Copyright 1985, 1987, Massachusetts Institute of Technology */

/* Beware, here be monsters (still under construction... - JG */

#define NEED_EVENTS
#include "Xlib.h"
#include "Xlibint.h"
#include "Xutil.h"
#include "keysym.h"
#include <stdio.h>

#define HAS_CTRL(c)  ((c) >= '@' && (c) <= '\177')

struct XKeytrans {
	struct XKeytrans *next;/* next on list */
	char *string;		/* string to return when the time comes */
	int len;		/* length of string (since NULL is legit)*/
	KeySym key;		/* keysym rebound */
	unsigned int state;	/* modifier state */
	KeySym *modifiers;	/* modifier keysyms you want */
	int mlen;		/* length of modifier list */
};

static struct XKeytrans *trans = NULL;

static KeySym KeyCodetoKeySym(dpy, keycode, col)
     register Display *dpy;
     int keycode;
     int col;
{
     int ind;
     /*
      * if keycode not defined in set, this should really be impossible.
      * in any case, if sanity check fails, return NoSymbol.
      */
     if (col < 0 || col > dpy->keysyms_per_keycode) return (NoSymbol);
     if (keycode < dpy->min_keycode || keycode > dpy->max_keycode) 
       return(NoSymbol);

     ind = (keycode - dpy->min_keycode) * dpy->keysyms_per_keycode + col;
     return (dpy->keysyms[ind]);
}

KeySym XKeycodeToKeysym(dpy, kc, col)
    Display *dpy;
    KeyCode kc;
    int col;
{
     if (dpy->keysyms == NULL)
         Initialize(dpy);
     return (KeyCodetoKeySym(dpy, kc, col));
}

KeyCode XKeysymToKeycode(dpy, ks)
    Display *dpy;
    KeySym ks;
{
    int         i;

     if (dpy->keysyms == NULL)
         Initialize(dpy);
    for (i = dpy->min_keycode; i <= dpy->max_keycode; i++) {
	int         j;

	for (j = 0; j < dpy->keysyms_per_keycode; j++) {
	    int ind = (i - dpy->min_keycode) * dpy->keysyms_per_keycode + j;

	    if (ks == dpy->keysyms[ind])
		return (i);
	}
    }
    return (0);
}

KeySym XLookupKeysym(event, col)
     register XKeyEvent *event;
     int col;
{
     if (event->display->keysyms == NULL)
         Initialize(event->display);
     return (XKeycodeToKeysym(event->display, event->keycode, col));
}

XRefreshKeyboardMapping(event)
     register XMappingEvent *event;
{
     LockDisplay(event->display);
     /* XXX should really only refresh what is necessary, for now, make
	initialize test fail */
     if(event->request == MappingKeyboard) 
	    if (event->display->keysyms != NULL) {
	         Xfree ((char *)event->display->keysyms);
	         event->display->keysyms = NULL;
	    }
     if(event->request == MappingModifier) {
	    XFreeModifiermap(event->display->modifiermap);
	    event->display->keysyms = NULL;/* XXX - looks like astorage leak */
     }
     UnlockDisplay(event->display);
}
static InitTranslationList()
{
	/* not yet implemented */
	/* should read keymap file and initialize list */
}

/*ARGSUSED*/
int XUseKeymap(filename) 
    char *filename;
{
  /* not yet implemented */
}

/* XXX not sure locking is race free here */
static Initialize(dpy)
Display *dpy;
{
    register KeySym *bd;
    int nbd;

    if (trans == NULL) InitTranslationList();
    /* 
     * lets go get the keysyms from the server.
     */
    if (dpy->keysyms == NULL) {
	dpy->keysyms = XGetKeyboardMapping (dpy, dpy->min_keycode,
	dpy->max_keycode - dpy->min_keycode + 1, &dpy->keysyms_per_keycode);
	LockDisplay(dpy);
	nbd = (dpy->max_keycode - dpy->min_keycode + 1) * dpy->keysyms_per_keycode;
	for (bd = dpy->keysyms; bd < (dpy->keysyms + nbd); bd += 2) {
	if ((*(bd + 1) == NoSymbol) && (*bd >= XK_A) && (*bd <= XK_Z)) {
	    *(bd + 1) = *bd;
	    *bd += 0x20;
	    }
	}
	UnlockDisplay(dpy);
    }
    if (dpy->modifiermap == NULL) {
	dpy->modifiermap = XGetModifierMapping(dpy);
    }
}

static int KeySymRebound(event, buf, symbol)
    XKeyEvent *event;
    char *buf;
    KeySym symbol;
{
    register struct XKeytrans *p;

    p = trans;
    while (p != NULL) {
	if (MatchEvent(event, symbol, p)) {
		bcopy (p->string, buf, p->len);
		return p->len;
		}
	p = p->next;
	}
    return -1;
}
  
Bool MatchEvent(event, symbol, p)
    XKeyEvent *event;
    KeySym symbol;
    register struct XKeytrans *p;
{
    if ((event->state == p->state) && (symbol == p->key)) return True;
    return False;
}

int XLookupString (event, buffer, nbytes, keysym, status)
     XKeyEvent *event;
     char *buffer;	/* buffer */
     int nbytes;	/* space in buffer for characters */
     KeySym *keysym;
     XComposeStatus *status;
{
     register KeySym symbol, lsymbol, usymbol;
     int length = 0;
     char buf[BUFSIZ];
     unsigned char byte3, byte4;

     if (event->display->keysyms == NULL)
         Initialize(event->display);

     lsymbol =  XKeycodeToKeysym(event->display, event->keycode, 0);
     usymbol =  XKeycodeToKeysym(event->display, event->keycode, 1);
     /*
      * we have to find out what kind of lock we are dealing with, if any.
      * if caps lock, only shift caps.
      */
     symbol = lsymbol;
     if (event->state & LockMask) {
	XModifierKeymap *m = event->display->modifiermap;
	int i;

	if (usymbol != NoSymbol)
	    symbol = usymbol;
	for (i = m->max_keypermod; i < 2*m->max_keypermod; i++) {
	    /*
	     *	Run through all the keys setting LOCK and,  if
	     *  ANY of them are CAPS_LOCK,  do Caps Lock.
	     *  This is kind of bogus,  but what else to do?
	     *  Supposing we have CAPS_LOCK,  but on the shifted
	     *  part of the key?
	     */
	    if (XKeycodeToKeysym(event->display, m->modifiermap[i], 0)
		    == XK_Caps_Lock) {
			if (usymbol >= XK_A && usymbol <= XK_Z)
			    symbol = usymbol;
			else
			    symbol = lsymbol;
			break;
		    }
	}
     }
     if ((event->state & ShiftMask) && usymbol != NoSymbol)
	     symbol = usymbol;

     if (keysym != NULL) *keysym = symbol;

     byte4 = symbol & 0xFF;
     byte3 = (symbol >> 8 ) & 0xFF;

     /*
      * see if symbol rebound, if so, return that string.
      * if any of high order 16 bits set, can only do ascii.
      * (reserved for future use, and for vendors).
      */
     if ((length = KeySymRebound(event, buf, symbol)) == -1) {
	    if ( IsModifierKey(symbol)   || IsCursorKey(symbol)
		|| IsPFKey (symbol)      || IsFunctionKey(symbol)
		|| IsMiscFunctionKey(symbol)
		|| (symbol == XK_Multi_key) || (symbol == XK_Kanji))  return 0;
            buf[0] = byte4;
	    /* if X keysym, convert to ascii by grabbing low 7 bits */
	    if (byte3 == 0xFF) buf[0] &= 0x7F;
	    /* only apply Control key if it makes sense, else ignore it */
	    if ((event->state & ControlMask) && HAS_CTRL(buf[0]))
	        buf[0] = buf[0] & 0x1F;
     	    length = 1;
      }
      if (length > nbytes) length = nbytes;
      bcopy (buf, buffer, length);
      return (length);
}

XRebindKeysym (dpy, keysym, mlist, nm, str, nbytes)
    Display *dpy;
    KeySym keysym;
    KeySym *mlist;
    int nm;		/* number of modifiers in mlist */
    unsigned char *str;
    int nbytes;
{
    register struct XKeytrans *tmp, *p;
    int nb;

    if (dpy->keysyms == NULL)
    	Initialize(dpy);
    LockDisplay(dpy);
    tmp = trans;
    trans = p = (struct XKeytrans *)Xmalloc(sizeof(struct XKeytrans));
    p->next = tmp;	/* chain onto list */
    p->string = (char *) Xmalloc(nbytes);
    bcopy (str, p->string, nbytes);
    p->len = nbytes;
    nb = sizeof (KeySym) * nm;
    p->modifiers = (KeySym *) Xmalloc(nb);
    bcopy (mlist, p->modifiers, nb);
    p->key = keysym;
    p->mlen = nm;
    ComputeMaskFromKeytrans(dpy, p);
    UnlockDisplay(dpy);
    return;
}

/*
 * given a KeySym, returns the first keycode found after the index value
 * in the table.  (Hopefully, can be found quickly).
 */
static CARD8 FindKeyCode(dpy, ind, code)
    register Display *dpy;
    int ind;
    register int code;
{

    register KeySym *kmax = dpy->keysyms + 
	(dpy->max_keycode - dpy->min_keycode + 1) * dpy->keysyms_per_keycode;
    register KeySym *k = dpy->keysyms;	/* XXX not yet dealing with ind */
    if ((ind < dpy->min_keycode) || (ind > dpy->max_keycode)) return 0;
    while (k < kmax) {
	if (*k == code)
	    return(((k - dpy->keysyms)
		/ dpy->keysyms_per_keycode) + dpy->min_keycode);
	k += 1;
	}
    return 0;
}

	
/*
 * given a list of modifiers, computes the mask necessary for later matching.
 * This routine must lookup the key in the Keymap and then search to see
 * what modifier it is bound to, if any.
 */
static ComputeMaskFromKeytrans(dpy, p)
    Display *dpy;
    register struct XKeytrans *p;
{
    register int i;
    register CARD8 code;
    register XModifierKeymap *m = dpy->modifiermap;

    p->state = 0;
    for (i = 0; i < p->mlen; i++) {
	/* if not found, then not on current keyboard */
	if ((code = FindKeyCode(dpy, dpy->min_keycode, p->modifiers[i])) == 0)
		continue;
	/* code is now the keycode for the modifier you want */
	{
	    register int j;

	    for (j = 0; j < (m->max_keypermod<<3); j++) {
		if (m->modifiermap[j] && code == m->modifiermap[j])
		    p->state |= (1<<(j/m->max_keypermod));
	    }
	}
    }
    return;
}
