#include "copyright.h"

/* $Header: XSynchro.c,v 11.5 87/09/11 08:07:45 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"


int _XSyncFunction(dpy)
register Display *dpy;
{
	XSync(dpy,0);
}

int (*XSynchronize(dpy,onoff))()
     register Display *dpy;
     int onoff;
{
        int (*temp)();

	LockDisplay(dpy);
	temp = dpy->synchandler;
	if (onoff) dpy->synchandler = _XSyncFunction;
	else dpy->synchandler = NULL;
	UnlockDisplay(dpy);
	return (temp);
}

int (*XSetAfterFunction(dpy,func))()
     register Display *dpy;
     int (*func)();
{
        int (*temp)();

	LockDisplay(dpy);
	temp = dpy->synchandler;
	dpy->synchandler = func;
	UnlockDisplay(dpy);
	return (temp);
}

