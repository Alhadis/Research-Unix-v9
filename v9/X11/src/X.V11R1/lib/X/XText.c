#include "copyright.h"

/* $Header: XText.c,v 11.14 87/08/29 13:46:46 newman Locked $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDrawString(dpy, d, gc, x, y, string, length)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y;
    char *string;
    int length;
{   
    int Datalength = 0;
    register xPolyText8Req *req;

    if (length <= 0)
       return;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq (PolyText8, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->x = x;
    req->y = y;


    Datalength += sizeof(xTextElt) * ((length + 253) / 254) + length;


    req->length += (Datalength + 3)>>2;  /* convert to number of 32-bit words */


    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.   If the request does fit into the
     * empty buffer, then we won't have to flush it at the end to keep
     * the buffer 32-bit aligned. 
     */

    if (dpy->bufptr + Datalength > dpy->bufmax)
    	_XFlush (dpy);

    {
	int nbytes;
	int PartialNChars = length;
        register xTextElt *elt;
 	char *CharacterOffset = string;

	while(PartialNChars > 254)
        {
 	    nbytes = 254 + sizeof(xTextElt);
	    BufAlloc (xTextElt *, elt, nbytes);
	    elt->delta = 0;
	    elt->len = 254;
            bcopy (CharacterOffset, (char *) (elt + 1), 254);
	    PartialNChars = PartialNChars - 254;
	    CharacterOffset += 254;
	}
	    
        if (PartialNChars)
        {
	    nbytes = PartialNChars + sizeof(xTextElt);
	    BufAlloc (xTextElt *, elt, nbytes); 
	    elt->delta = 0;
	    elt->len = PartialNChars;
            bcopy (CharacterOffset, (char *) (elt + 1), PartialNChars);
	 }
    }

    /* Pad request out to a 32-bit boundary */

    if (Datalength &= 3) {
	char *pad;
	/* 
	 * BufAlloc is a macro that uses its last argument more than
	 * once, otherwise I'd write "BufAlloc (char *, pad, 4-length)" 
	 */
	length = 4 - Datalength;
	BufAlloc (char *, pad, length);
	/* 
	 * if there are 3 bytes of padding, the first byte MUST be 0
	 * so the pad bytes aren't mistaken for a final xTextElt 
	 */
	*pad = 0;
        }

    /* 
     * If the buffer pointer is not now pointing to a 32-bit boundary,
     * we must flush the buffer so that it does point to a 32-bit boundary
     * at the end of this routine. 
     */

    if ((dpy->bufptr - dpy->buffer) & 3)
       _XFlush (dpy);
    UnlockDisplay(dpy);
    SyncHandle();
    return;
}

