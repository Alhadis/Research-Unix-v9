/* $Header: XStBytes.c,v 11.13 87/09/01 15:06:39 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"
#include "Xatom.h"

/* insulate predefined atom numbers from cut routines */
static Atom n_to_atom[8] = { 
	XA_CUT_BUFFER0,
	XA_CUT_BUFFER1,
	XA_CUT_BUFFER2,
	XA_CUT_BUFFER3,
	XA_CUT_BUFFER4,
	XA_CUT_BUFFER5,
	XA_CUT_BUFFER6,
	XA_CUT_BUFFER7};

XRotateBuffers (dpy, rotate)
    register Display *dpy;
    int rotate;
{
	XRotateWindowProperties(dpy, RootWindow(dpy, 0), n_to_atom, 8, rotate);
}
    
char *XFetchBuffer (dpy, nbytes, buffer)
    register Display *dpy;
    int *nbytes;
    register int buffer;
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    long leftover;
    unsigned char *data;
    *nbytes = 0;
    if ((buffer < 0) || (buffer > 7)) return (NULL);
/* XXX should be (sizeof (maxint) - 1)/4 */
    if (XGetWindowProperty(dpy, RootWindow(dpy, 0), n_to_atom[buffer], 
	0L, 10000000L, False, XA_STRING, 
	&actual_type, &actual_format, &nitems, &leftover, &data) != Success) {
	return (NULL);
	}
    if ( (actual_type == XA_STRING) &&  (actual_format != 32) ) {
	*nbytes = nitems;
	return((char *)data);
	}
    if ((char *) data != NULL) Xfree ((char *)data);
    return(NULL);
}

char *XFetchBytes (dpy, nbytes)
    register Display *dpy;
    int *nbytes;
{
    return (XFetchBuffer (dpy, nbytes, 0));
}

XStoreBuffer (dpy, bytes, nbytes, buffer)
    register Display *dpy;
    char bytes[];
    int nbytes;
    register int buffer;
{
    if ((buffer < 0) || (buffer > 7)) return;
    XChangeProperty(dpy, RootWindow(dpy, 0), n_to_atom[buffer], 
	XA_STRING, 8, PropModeReplace, (unsigned char *) bytes, nbytes);
}

XStoreBytes (dpy, bytes, nbytes)
    register Display *dpy;
    char bytes[];
    int nbytes;
{
    XStoreBuffer (dpy, bytes, nbytes, 0);
}
