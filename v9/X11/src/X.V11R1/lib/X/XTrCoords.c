#include "copyright.h"

/* $Header: XTrCoords.c,v 11.10 87/09/11 08:07:48 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

int XTranslateCoordinates(dpy, src_win, dest_win, src_x, src_y, 
		      dst_x, dst_y, child)
     register Display *dpy;
     Window src_win, dest_win;
     int src_x, src_y;
     int *dst_x, *dst_y;
     Window *child;
{       
    register xTranslateCoordsReq *req;
    xTranslateCoordsReply rep;

    LockDisplay(dpy);
    GetReq(TranslateCoords, req);
    req->srcWid = src_win;
    req->dstWid = dest_win;
    req->srcX = src_x;
    req->srcY = src_y;
    if (_XReply (dpy, (xReply *)&rep, 0, xTrue) == 0) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return(False);
	}
	
    *child = rep.child;
    *dst_x = rep.dstX;
    *dst_y = rep.dstY;
    UnlockDisplay(dpy);
    SyncHandle();
    return ((int)rep.sameScreen);
}

