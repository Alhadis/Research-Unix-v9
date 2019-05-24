#include "copyright.h"

/* $Header: XRotProp.c,v 11.9 87/09/11 08:08:55 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XRotateWindowProperties(dpy, w, properties, nprops, npositions)
    register Display *dpy;
    Window w;
    Atom properties[];
    register int nprops;
    int npositions;
    {
    register long nbytes;
    register xRotatePropertiesReq *req;

    LockDisplay(dpy);
    GetReq (RotateProperties, req);
    req->window = w;
    req->nAtoms = nprops;
    req->nPositions = npositions;
    
    req->length += nprops;
    nbytes = nprops << 2;
/* XXX Cray needs packing here.... */
    Data (dpy, (char *) properties, nbytes);


    UnlockDisplay(dpy);
    SyncHandle();
    }





