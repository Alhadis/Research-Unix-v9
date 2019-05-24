#include "copyright.h"

/* $Header: XChProp.c,v 11.12 87/09/11 08:01:37 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XChangeProperty (dpy, w, property, type, format, mode, data, nelements)
    register Display *dpy;
    Window w;
    Atom property, type;
    int format;  /* 8, 16, or 32 */
    int mode;  /* PropModeReplace, PropModePrepend, PropModeAppend */
    unsigned char *data;
    int nelements;
    {
    register xChangePropertyReq *req;
    register long nbytes = nelements;

    LockDisplay(dpy);
    GetReq (ChangeProperty, req);
    req->window = w;
    req->property = property;
    req->type = type;
    req->format = format;
    req->mode = mode;
    req->nUnits = nelements;
    
    switch (format) {
      case 8:
	req->length += (nelements + 3)>>2;
	Data (dpy, (char *)data, nbytes);
        break;
 
      case 16:
	req->length += (nelements + 1)>>1;
	nbytes <<= 1;
	PackData (dpy, (char *) data, nbytes);
	break;

      case 32:
	req->length += nelements;
	nbytes <<= 2;
	Data (dpy, (char *) data, nbytes);
	break;

      default:
        /* XXX this is an error! */ ;
      }

    UnlockDisplay(dpy);
    SyncHandle();
    }





