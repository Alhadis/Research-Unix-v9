#include "copyright.h"

/* $Header: XStName.c,v 11.9 87/09/11 08:07:39 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"
#include "Xatom.h"

XStoreName (dpy, w, name)
    register Display *dpy;
    Window w;
    char *name;
{
    XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, 
		8, PropModeReplace, (unsigned char *)name, strlen(name));
}

XSetIconName (dpy, w, icon_name)
    register Display *dpy;
    Window w;
    char *icon_name;
{
    XChangeProperty(dpy, w, XA_WM_ICON_NAME, XA_STRING, 
		8, PropModeReplace, (unsigned char *)icon_name, strlen(icon_name));
}
