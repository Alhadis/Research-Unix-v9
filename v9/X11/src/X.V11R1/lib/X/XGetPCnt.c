#include "copyright.h"

/* $Header: XGetPCnt.c,v 11.9 87/09/11 08:04:20 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

XGetPointerControl(dpy, accel_numer, accel_denom, threshold)
     register Display *dpy;
     /* the following are return only vars */
     int *accel_numer, *accel_denom;
     int *threshold;
{       
    xGetPointerControlReply rep;
    xReq *req;
    LockDisplay(dpy);
    GetEmptyReq(GetPointerControl, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    *accel_numer = rep.accelNumerator;
    *accel_denom = rep.accelDenominator;
    *threshold = rep.threshold;
    UnlockDisplay(dpy);
    SyncHandle();
}

