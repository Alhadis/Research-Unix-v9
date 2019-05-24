#include "copyright.h"

/* $Header: XImText16.c,v 11.13 87/09/01 14:52:38 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDrawImageString16(dpy, d, gc, x, y, string, length)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y;
    XChar2b *string;
    int length;
{   
    register xImageText16Req *req;
    XChar2b *CharacterOffset = string;
    int FirstTimeThrough = True;
    int lastX = 0;

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    while (length > 0) 
    {
	int Unit, Datalength;

	if (length > 255) Unit = 255;
	else Unit = length;

   	if (FirstTimeThrough)
	{
	    FirstTimeThrough = False;
        }
	else
	{
	    int direction, ascent, descent;
	    XCharStruct overall;
	    XFontStruct *FontStruct;
            
	    UnlockDisplay(dpy);
	
	    FontStruct = XQueryFont(dpy, gc->gid);

  	    XTextExtents16(FontStruct, CharacterOffset - 255, 255,
		&direction, &ascent, &descent, &overall);

	    Xfree(FontStruct);

	    LockDisplay(dpy);

	    x = lastX + overall.width;
	}


        GetReq (ImageText16, req);
        req->length += ((Unit << 1) + 3) >> 2;
        req->nChars = Unit;
        req->drawable = d;
        req->gc = gc->gid;
        req->y = y;

	lastX = req->x = x;
	Datalength = Unit << 1;
        Data (dpy, (char *)CharacterOffset, (long)Datalength);
        CharacterOffset += Unit;
	length -= Unit;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

