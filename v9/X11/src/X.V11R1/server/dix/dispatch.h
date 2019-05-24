/* $Header: dispatch.h,v 1.1 87/09/11 07:18:44 toddb Exp $ */
#ifndef DISPATCH_H
#define DISPATCH_H


#define LEGAL_CLIENT(id, c) (\
    (c == CLIENT_ID(id)) && (clientUsed[CLIENT_ID(id)].used)) 

#define REQUEST_AT_LEAST_SIZE(req) \
    if ((sizeof(req) >> 2) > stuff->length )\
         return(BadLength)

#define REQUEST_SIZE_MATCH(req)\
    if ((sizeof(req) >> 2) != stuff->length)\
         return(BadLength)

extern int ProcBell();
extern int ProcChangeActivePointerGrab();
extern int ProcChangeKeyboardControl();
extern int ProcChangePointerControl();
extern int ProcGetDeviceMapping();
extern int ProcGetInputFocus();
extern int ProcGetKeyboardControl();
extern int ProcGetMotionEvents();
extern int ProcGetPointerControl();
extern int ProcGrabKey();
extern int ProcGrabKeyboard();
extern int ProcGrabPointer();
extern int ProcQueryKeymap();
extern int ProcQueryPointer();
extern int ProcSetDeviceMapping();
extern int ProcSetInputFocus();
extern int ProcSendEvent();
extern int ProcUngrabKey();
extern int ProcUngrabKeyboard();
extern int ProcUngrabPointer();
extern int ProcWarpPointer();

#endif /* DISPATCH_H */

extern int curclient;
extern int ErrorResID;
extern int *RequestSequenceNumber;
