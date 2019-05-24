#include "copyright.h"

/* $Header: XGetProp.c,v 11.10 87/09/11 08:04:23 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

int
XGetWindowProperty(dpy, w, property, offset, length, delete, 
	req_type, actual_type, actual_format, nitems, bytesafter, prop)
    register Display *dpy;
    Window w;
    Atom property;
    Bool delete;
    Atom req_type;
    Atom *actual_type;
    int *actual_format;  /* 8, 16, or 32 */
    long offset, length;
    unsigned long *nitems;  /* # of 8-, 16-, or 32-bit entities */
    long *bytesafter;
    unsigned char **prop;
    {
    int errorstatus;
    xGetPropertyReply reply;
    register xGetPropertyReq *req;
    LockDisplay(dpy);
    GetReq (GetProperty, req);
    req->window = w;
    req->property = property;
    req->type = req_type;
    req->delete = delete;
    req->longOffset = offset;
    req->longLength = length;
    
    if (!(errorstatus = _XReply (dpy, (xReply *) &reply, 0, xFalse))) {
	UnlockDisplay(dpy);
	return (errorstatus);
	}	
    *actual_type = reply.propertyType;
    *actual_format = reply.format;
    *nitems = reply.nItems;
    *bytesafter = reply.bytesAfter;

    *prop = NULL;
    if (*nitems) switch (reply.format) {
      long nbytes;
      /* 
       * One extra byte is malloced than is needed to contain the property
       * data, but this last byte is null terminated and convenient for 
       * returing string properties, so the client doesn't then have to 
       * recopy the string to make it null terminated. 
       */
      case 8:
	*prop = (unsigned char *) Xmalloc ((unsigned)reply.nItems + (unsigned)1);
        _XReadPad (dpy, (char *) *prop, (long) reply.nItems);
	(*prop)[reply.nItems] = '\0';
        break;

      case 16:
        /* XXX needs rethinking for BIGSHORTS */
        nbytes = (long)reply.nItems << 1;
        *prop = (unsigned char *) Xmalloc ((unsigned)nbytes + 1);
        _XReadPad (dpy, (char *) *prop, nbytes);
	(*prop)[nbytes] = '\0';
        break;

      case 32:
        nbytes = (long)reply.nItems << 2;
        *prop = (unsigned char *) Xmalloc ((unsigned)nbytes + 1);
        _XRead (dpy, (char *) *prop, (long)nbytes);
	(*prop)[nbytes] = '\0';
        break;

      default:
	/*
	 * This part of the code should never be reached.  If it is, the server
	 * send back a property with an invalid format.  This is a
	 * BadImplementation error. 
	 */

        {
	    xError error;

	    error.sequenceNumber = dpy->request;
  	    error.type = X_Error;
	    error.majorCode = X_GetProperty;
	    error.minorCode = 0;
	    error.errorCode = BadImplementation;
	
	    _XError(dpy, &error);
	}
	break;
      }
    UnlockDisplay(dpy);
    SyncHandle();
    return(Success);

    }

