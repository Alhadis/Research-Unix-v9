#include "copyright.h"

/* $Header: XAutoRep.c,v 11.7 87/09/11 08:01:02 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#include "Xlibint.h"
XAutoRepeatOn (dpy)
register Display *dpy;
{
	XKeyboardControl values;
	values.auto_repeat_mode = AutoRepeatModeOn;
	XChangeKeyboardControl (dpy, KBAutoRepeatMode, &values);
}

XAutoRepeatOff (dpy)
register Display *dpy;
{
	XKeyboardControl values;
	values.auto_repeat_mode = AutoRepeatModeOff;
	XChangeKeyboardControl (dpy, KBAutoRepeatMode, &values);
}


