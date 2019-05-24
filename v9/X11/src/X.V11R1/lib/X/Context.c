/* $Header: Context.c,v 1.1 87/09/11 08:16:18 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Context.c	1.5	2/24/87";
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


/* Created by weissman, Thu Jun 26 15:18:59 1986 */

/* This module implements a simple sparse array.

   XSaveContext(a,b,c,d) will store d in position (a,b,c) of the array.
   XFindContext(a,b,c,&d) will set d to be the value in position (a,b,c).
   XDeleteContext(a,b,c) will delete the entry in (a,b,c).

   a is a display id, b is a window id, and c is a Context.  d is just a
   caddr_t.  This code will work with any range of parameters, but is geared
   to be most efficient with very few (one or two) different a's.

*/

#include <stdio.h>
#include "Xlib.h"
#include "Xutil.h"
#include "Xlibos.h"

#define INITHASHSIZE 1024 /* Number of entries originally in the hash table. */


typedef struct _TableEntryRec {	/* Stores one entry. */
    Window 			window;
    XContext			context;
    caddr_t			data;
    struct _TableEntryRec	*next;
} TableEntryRec, *TableEntry;

typedef struct _DspRec {	/* Stores hash table for one display. */
    Display *display;		/* Which display this is a hash for. */
    TableEntry *table;		/* Pointer to array of hash entries. */
    int size;			/* Current size of hash table. */
    int numentries;		/* Number of entries currently in table. */
    int maxentries;		/* How many entries we can take before
				   increasing table size. */
} DspRec, *Dsp;

static Dsp *DspArray = NULL;	/* Pointer to array of display entries. */
static int numDsp = 0;		/* How many display entries we have. */


/* Assign dsp to be the Dsp entry that contains display. */

static Dsp lastdsp = NULL;
#define FindCorrectDsp \
	if (lastdsp && display == lastdsp->display) dsp = lastdsp; \
	else dsp = lastdsp = FindDsp(display);


/* Given a Window and a context, returns a value between 0 and HashSize-1.
   Currently, this requires that HashSize be a power of 2.
*/

#define HashValue(window, context, HashSize) \
    ((((int)window << 3) + (int)context) & ((HashSize) - 1))


/* Resize the given dsp to have the given number of hash buckets. */

static int ResizeTable(dsp, NewSize)
Dsp dsp;
int NewSize;
{
    TableEntry *OldHashTable;
    register TableEntry CurEntry, NextEntry;
    register int i, OldHashSize, CurHash;
    OldHashTable = dsp->table;
    OldHashSize = dsp->size;
    dsp->table = (TableEntry *) Xcalloc((unsigned)NewSize,sizeof(TableEntry));
    if (dsp->table == NULL) {
	dsp->table = OldHashTable;
	return XCNOMEM;
    }
    dsp->size = NewSize;
    dsp->maxentries = NewSize; /* When to next resize the hash table. */
    if (OldHashTable != NULL) {
	for (i=0 ; i<OldHashSize ; i++) {
	    CurEntry = OldHashTable[i] ;
	    while (CurEntry != NULL) {
		(void) XDeleteContext(dsp->display,
				       CurEntry->window, CurEntry->context);
		NextEntry = CurEntry->next;
		CurHash = HashValue(CurEntry->window, CurEntry->context,
				    NewSize);
		CurEntry->next = dsp->table[CurHash];
		dsp->table[CurHash] = CurEntry;
		CurEntry = NextEntry;
	    }
	}
	Xfree((char *) OldHashTable);
    }
    return 0;
}


/* Given a display, find the corresponding dsp. */

static Dsp FindDsp(display)
Display *display;
{
    int i;
    Dsp dsp;
    for (i=0 ; i<numDsp ; i++)
	if (DspArray[i]->display == display) return DspArray[i];
    numDsp++;
    if (DspArray == NULL)
	DspArray = (Dsp *) Xmalloc(sizeof(Dsp));
    else
	DspArray = (Dsp *) Xrealloc(
		(char *)DspArray, (unsigned) sizeof(Dsp) * numDsp);
    dsp = DspArray[numDsp - 1] = (Dsp) Xmalloc(sizeof(DspRec));
    dsp->display = display;
    dsp->table = NULL;
    dsp->size = INITHASHSIZE / 2;
    dsp->numentries = 0;
    dsp->maxentries = -1;
    (void) ResizeTable(dsp, dsp->size * 2);
    return dsp;
}



/* Public routines. */

/* Save the given value of data to correspond with the keys Window and context.
   If an entry with the given Window and context already exists, this one will 
   override it; however, such an override has costs in time and space.  It
   is better to call XDeleteContext first if you know the entry already exists.
   Returns nonzero error code if an error has occured, 0 otherwise.
   Possible errors are Out-of-memory.
*/   

int XSaveContext(display, window, context, data)
register Display *display;
register Window window;
register XContext context;
caddr_t data;
{
    register int CurHash;
    register TableEntry CurEntry;
    register Dsp dsp;
    FindCorrectDsp;
    if (++(dsp->numentries) > dsp->maxentries) 
	if (ResizeTable(dsp, dsp->size * 2) == XCNOMEM)
	    return XCNOMEM;
    CurEntry = (TableEntry) Xmalloc(sizeof(TableEntryRec));
    if (CurEntry == NULL) return XCNOMEM;
    CurEntry->window = window;
    CurEntry->context = context;
    CurEntry->data = data;
    CurHash = HashValue(window, context, dsp->size);
    CurEntry->next = dsp->table[CurHash];
    dsp->table[CurHash] = CurEntry;
    return 0;
}



/* Given a window and context, returns the associated data.  Note that data 
   here is a pointer since it is a return value.  Returns nonzero error code
   if an error has occured, 0 otherwise.  Possible errors are Entry-not-found.
*/

int XFindContext(display, window, context, data)
register Display *display;
register Window window;
register XContext context;
caddr_t *data;			/* RETURN */
{
    register TableEntry CurEntry;
    register Dsp dsp;
    FindCorrectDsp;
    for (CurEntry = dsp->table[HashValue(window, context, dsp->size)];
	 CurEntry != NULL;
	 CurEntry = CurEntry->next)
    {
	if (CurEntry->window == window && CurEntry->context == context) {
	    *data = CurEntry->data;
	    return 0;
	}
    }
    return XCNOENT;
}



/* Deletes all entries for the given window and context from the datastructure.
   This returns the same thing that FindContext would have returned if called
   with the same arguments.
*/

int XDeleteContext(display, window, context)
register Display *display;
register Window window;
register XContext context;
{
    register int CurHash;
    int Result;
    register TableEntry CurEntry, PrevEntry, NextEntry;
    register Dsp dsp;

    FindCorrectDsp;
    Result = XCNOENT;
    CurHash = HashValue(window, context, dsp->size);
    PrevEntry = NULL;
    CurEntry = dsp->table[CurHash];
    while (CurEntry != NULL) {
	if (CurEntry->window == window && CurEntry->context == context) {
	    dsp->numentries--;
	    Result = 0;
	    NextEntry = CurEntry->next;
	    if (PrevEntry == NULL) {
		dsp->table[CurHash] = NextEntry;
	    } else {
		PrevEntry->next = NextEntry;
	    }
	    Xfree((char *) CurEntry);
	    CurEntry = NextEntry;
	} else {
	    PrevEntry = CurEntry;
	    CurEntry = CurEntry->next;
	}
    }
    return Result;
}
