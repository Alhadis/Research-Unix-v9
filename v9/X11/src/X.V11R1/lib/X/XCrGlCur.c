#include "copyright.h"

/* $Header: XCrGlCur.c,v 11.6 87/09/11 08:02:32 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

Cursor XCreateGlyphCursor(dpy, source_font, mask_font,
		   source_char, mask_char,
		   foreground, background)
     register Display *dpy;
     Font source_font, mask_font;
     unsigned int source_char, mask_char;
     XColor *foreground, *background;

{       
    Cursor cid;
    register xCreateGlyphCursorReq *req;

    LockDisplay(dpy);
    GetReq(CreateGlyphCursor, req);
    cid = req->cid = XAllocID(dpy);
    req->source = source_font;
    req->mask = mask_font;
    req->sourceChar = source_char;
    req->maskChar = mask_char;
    req->foreRed = foreground->red;
    req->foreGreen = foreground->green;
    req->foreBlue = foreground->blue;
    req->backRed = background->red;
    req->backGreen = background->green;
    req->backBlue = background->blue;
    UnlockDisplay(dpy);
    SyncHandle();
    return (cid);
}

