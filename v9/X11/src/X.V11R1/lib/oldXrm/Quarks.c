#ifndef lint
static char *sccsid = "@(#)Atom.c	1.21	3/19/87";
#endif lint

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */


/* File: Quarks.c - last edit by */

#include <sys/param.h>
#include "Xlib.h"
#include "Xlibos.h"
#include "Xresource.h"
#include "Quarks.h"
#include <strings.h>

extern void bcopy();


typedef int Signature;

static XrmQuark nextQuark = 1;	/* next available quark number */
static XrmAtom *quarkToAtomTable = NULL;
static int maxQuarks = 0;	/* max names in current quarkToAtomTable */
#define QUARKQUANTUM 300	/* how much to extend quarkToAtomTable by */

typedef struct _NodeRec *Node;
typedef struct _NodeRec {
    Node 	next;
    Signature	sig;
    XrmQuark	quark;
    XrmAtom	name;
} NodeRec;

#define HASHTABLESIZE 1000
static Node nodeTable[HASHTABLESIZE];



/* predefined quarks */

/* Representation types */

XrmQuark  XrmQBoolean;
XrmQuark  XrmQColor;
XrmQuark  XrmQCursor;
XrmQuark  XrmQDims;
XrmQuark  XrmQDisplay;
XrmQuark  XrmQFile;
XrmQuark  XrmQFont;
XrmQuark  XrmQFontStruct;
XrmQuark  XrmQGeometry;
XrmQuark  XrmQInt;
XrmQuark  XrmQPixel;
XrmQuark  XrmQPixmap;
XrmQuark  XrmQPointer;
XrmQuark  XrmQString;
XrmQuark  XrmQWindow;

/* "Enumeration" constants */

XrmQuark  XrmQEfalse;
XrmQuark  XrmQEno;
XrmQuark  XrmQEoff;
XrmQuark  XrmQEon;
XrmQuark  XrmQEtrue;
XrmQuark  XrmQEyes;


static XrmAllocMoreQuartToAtomTable()
{
    unsigned	size;

    maxQuarks += QUARKQUANTUM;
    size = (unsigned) maxQuarks * sizeof(XrmAtom);
    if (quarkToAtomTable == (XrmAtom *)NULL)
	quarkToAtomTable = (XrmAtom *) Xmalloc(size);
    else
	quarkToAtomTable =
		(XrmAtom *) Xrealloc((char *) quarkToAtomTable, size);
}

XrmQuark XrmAtomToQuark(name)
    register XrmAtom name;
{
    register Signature 	sig = 0;
    register Signature	scale = 27;
    register XrmAtom	tname;
    register Node	np;
    register XrmAtom	npn;
    	     Node	*hashp;
	     unsigned	strLength;

    if (name == NULL)
	return (NULLQUARK);

    /* Compute atom signature (sparse 31-bit hash value) */
    for (tname = name; *tname != '\0'; tname++)
	sig = sig*scale + (unsigned int) *tname;
    strLength = tname - name + 1;
    if (sig < 0)
        sig = -sig;

    /* Look for atom in hash table */
    hashp = &nodeTable[sig % HASHTABLESIZE];
    for (np = *hashp; np != NULL; np = np->next) {
	if (np->sig == sig) {
	    for (npn=np->name, tname = name;
	     ((scale = *tname) != 0) && (scale == *npn); ++tname, ++npn) {};
	    if (scale == *npn) {
	        return np->quark;
	    }
	}
    }

    /* Not found, add atom to hash table */
    np = (Node) Xmalloc (sizeof (NodeRec));
    np->next = *hashp;
    *hashp = np;
    np->sig = sig;
    bcopy(name, (np->name = Xmalloc(strLength)), (int) strLength);
    np->quark = nextQuark;

    if (nextQuark >= maxQuarks)
	XrmAllocMoreQuartToAtomTable();

    quarkToAtomTable[nextQuark] = np->name;
    ++nextQuark;
    return np->quark;
}

extern XrmQuark XrmUniqueQuark()
{
    XrmQuark quark;

    quark = nextQuark;
    if (nextQuark >= maxQuarks)
	XrmAllocMoreQuartToAtomTable();
    quarkToAtomTable[nextQuark] = NULLATOM;
    ++nextQuark;
    return (quark);
}


XrmAtom XrmQuarkToAtom(quark)
    XrmQuark quark;
{
    if (quark <= 0 || quark >= nextQuark)
    	return NULLATOM;
    return quarkToAtomTable[quark];
}


void XrmStringToQuarkList(name, quarks)
    register char 	 *name;
    register XrmQuarkList quarks;
{
    register int  i;
    static char oneName[1000];

    if (name != NULL) {
	for (i = 0 ; (name[0] != '\0') ; name++) {
	    if (*name == '.') {
		if (i == 0) {
		    /* Skip over leading . */
		} else {
		    oneName[i] = 0;
		    *quarks = XrmAtomToQuark(oneName);
		    quarks++;
		    i = 0;
		}
	    } else {
		oneName[i] = *name;
	    	i++;
	    }
	}
	if (i != 0) {
	    oneName[i] = 0;
	    *quarks = XrmAtomToQuark(oneName);
	    quarks++;
	}
    }
    *quarks = NULLQUARK;
}


XrmQuarkList XrmNewQuarkList()
{
    register XrmQuarkList result;

    result = (XrmQuarkList) Xmalloc(sizeof(XrmQuark));
    *result = NULLQUARK;
    return result;
}


/* ||| Could be replaced by define in Xresource.h, but Xfree being a macro
   complicates things. */

XrmQuarkList XrmFreeQuarkList(list)
  XrmQuarkList list;
{
    Xfree((char *) list);
}



/* Return the number of elements in an atomlist. */

int XrmQuarkListLength(list)
  register XrmQuarkList list;
{
    register int result;

    for (result = 0; *list != NULLQUARK ; result++, list++) ;
    return result;
}


XrmQuarkList XrmCopyQuarkList(list)
    register XrmQuarkList list;
{
    register int length;
    register XrmQuarkList result, dest;

    length = XrmQuarkListLength(list)+1;
    result = (XrmQuarkList) Xcalloc((unsigned) length, sizeof(XrmQuark));
    for (dest = result; --length >= 0 ; dest++, list++)
    	*dest = *list;
    return result;
}


void QuarkInitialize()
{
/* Representation types */

    XrmQBoolean		= XrmAtomToQuark(XrmRBoolean);
    XrmQColor		= XrmAtomToQuark(XrmRColor);
    XrmQCursor		= XrmAtomToQuark(XrmRCursor);
    XrmQDims		= XrmAtomToQuark(XrmRDims);
    XrmQDisplay		= XrmAtomToQuark(XrmRDisplay);
    XrmQFile		= XrmAtomToQuark(XrmRFile);
    XrmQFont		= XrmAtomToQuark(XrmRFont);
    XrmQFontStruct	= XrmAtomToQuark(XrmRFontStruct);
    XrmQGeometry	= XrmAtomToQuark(XrmRGeometry);
    XrmQInt		= XrmAtomToQuark(XrmRInt);
    XrmQPixel		= XrmAtomToQuark(XrmRPixel);
    XrmQPixmap		= XrmAtomToQuark(XrmRPixmap);
    XrmQPointer		= XrmAtomToQuark(XrmRPointer);
    XrmQString		= XrmAtomToQuark(XrmRString);
    XrmQWindow		= XrmAtomToQuark(XrmRWindow);

/* Boolean enumeration constants */

    XrmQEfalse		= XrmAtomToQuark(XrmEfalse);
    XrmQEno		= XrmAtomToQuark(XrmEno);
    XrmQEoff		= XrmAtomToQuark(XrmEoff);
    XrmQEon		= XrmAtomToQuark(XrmEon);
    XrmQEtrue		= XrmAtomToQuark(XrmEtrue);
    XrmQEyes		= XrmAtomToQuark(XrmEyes);
}
