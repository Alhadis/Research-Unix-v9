/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: dix.h,v 1.49 87/08/14 22:48:17 newman Exp $ */

#ifndef DIX_H
#define DIX_H

#include "gc.h"
#include "window.h"

#define EARLIER -1
#define SAMETIME 0
#define LATER 1

#define NullClient ((ClientPtr) 0)
#define REQUEST(type) \
	register type *stuff = (type *)client->requestBuffer


#define REQUEST_SIZE_MATCH(req)\
    if ((sizeof(req) >> 2) != stuff->length)\
         return(BadLength)

#define REQUEST_AT_LEAST_SIZE(req) \
    if ((sizeof(req) >> 2) > stuff->length )\
         return(BadLength)

#define WriteReplyToClient(pClient, size, pReply) \
   if (pClient->swapped) \
      (*ReplySwapVector[((xReq *)pClient->requestBuffer)->reqType]) \
           (pClient, size, pReply); \
      else WriteToClient(pClient, size, (char *) pReply);

#define WriteSwappedDataToClient(pClient, size, pbuf) \
   if (pClient->swapped) \
      (*pClient->pSwapReplyFunc)(pClient, size, pbuf); \
   else WriteToClient (pClient, size, (char *) pbuf);

typedef struct _TimeStamp *TimeStampPtr;
typedef struct _Client *ClientPtr;
extern ClientPtr requestingClient;
extern ClientPtr *clients;
extern ClientPtr serverClient;
extern int currentMaxClients;

extern int ProcAllowEvents();
extern int ProcBell();
extern int ProcChangeActivePointerGrab();
extern int ProcChangeKeyboardControl();
extern int ProcChangePointerControl();
extern int ProcGetKeyboardMapping();
extern int ProcGetPointerMapping();
extern int ProcGetInputFocus();
extern int ProcGetKeyboardControl();
extern int ProcGetMotionEvents();
extern int ProcGetPointerControl();
extern int ProcGrabButton();
extern int ProcGrabKey();
extern int ProcGrabKeyboard();
extern int ProcGrabPointer();
extern int ProcQueryKeymap();
extern int ProcQueryPointer();
extern int ProcSetInputFocus();
extern int ProcSetKeyboardMapping();
extern int ProcSetPointerMapping();
extern int ProcSendEvent();
extern int ProcUngrabButton();
extern int ProcUngrabKey();
extern int ProcUngrabKeyboard();
extern int ProcUngrabPointer();
extern int ProcWarpPointer();
extern int ProcRecolorCursor();

extern WindowPtr LookupWindow();
extern pointer LookupDrawable();

extern void NoopDDA();

#endif /* DIX_H */
