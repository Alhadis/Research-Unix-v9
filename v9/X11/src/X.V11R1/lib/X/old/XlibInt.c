#include "copyright.h"
/* Copyright    Massachusetts Institute of Technology    1985, 1986, 1987 */

#ifndef lint
static char rcsid[] = "$Header: XlibInt.c,v 11.55 87/08/20 17:25:23 newman Exp $";
#endif

/*
 *	XlibInternal.c - Internal support routines for the C subroutine
 *	interface library (Xlib) to the X Window System Protocol V11.0.
 */
#define NEED_EVENTS
#define NEED_REPLIES

#include <stdio.h>
#include "Xlibint.h"

/*
 * The following routines are internal routines used by Xlib for protocol
 * packet transmission and reception.
 *
 * XIOError(Display *) will be called if any sort of system call error occurs.
 * This is assumed to be a fatal condition, i.e., XIOError should not return.
 *
 * XError(Display *, XErrorEvent *) will be called whenever an X_Error event is
 * received.  This is not assumed to be a fatal condition, i.e., it is
 * acceptable for this procedure to return.  However, XError should NOT
 * perform any operations (directly or indirectly) on the DISPLAY.
 *
 * Routines declared with a return type of 'Status' return 0 on failure,
 * and non 0 on success.  Routines with no declared return type don't 
 * return anything.  Whenever possible routines that create objects return
 * the object they have created.
 */

_XQEvent *_qfree = NULL;			/* NULL _XQEvent. */

static int padlength[4] = {0, 3, 2, 1};
    /* lookup table for adding padding bytes to data that is read from
    	or written to the X socket.  */

static xReq _dummy_request = {
	0, 0, 0
};
/*
 * _XFlush - Flush the X request buffer.  If the buffer is empty, no
 * action is taken.  This routine correctly handles incremental writes.
 * This routine may have to be reworked if int < long.
 */
_XFlush (dpy)
	register Display *dpy;
{
	register long size;
	register int write_stat;
	register char *bufindex;

	size = dpy->bufptr - dpy->buffer;
	bufindex = dpy->bufptr = dpy->buffer;
	/*
	 * While write has not written the entire buffer, keep looping
	 * until the entire buffer is written.  bufindex will be incremented
	 * and size decremented as buffer is written out.
	 */
	while (size) {
	    write_stat = WriteToServer(dpy->fd, bufindex, (int) size);
	    if (write_stat > 0) {
		size -= write_stat;
		bufindex += write_stat;
	    } else if (errno == EWOULDBLOCK) {
		_XWaitForWritable(dpy);
	    } else {
		/* Write failed! */
		/* errno set by write system call. */
		(*_XIOErrorFunction)(dpy);
	    }
	}
	dpy->last_req = (char *)&_dummy_request;
}

/* 
 * _XRead - Read bytes from the socket taking into account incomplete
 * reads.  This routine may have to be reworked if int < long.
 */
_XRead (dpy, data, size)
	register Display *dpy;
	register char *data;
	register long size;
{
	register long bytes_read;

	if (size == 0) return;
	while ((bytes_read = ReadFromServer(dpy->fd, data, (int)size))
		!= size) {

	    	if (bytes_read > 0) {
		    size -= bytes_read;
		    data += bytes_read;
		    }

		else if (errno == EWOULDBLOCK) {
		    _XWaitForReadable(dpy);
		    errno = 0;
		}
		
		else if (bytes_read == 0) {
		    /* Read failed because of end of file! */
		    errno = EPIPE;
		    (*_XIOErrorFunction)(dpy);
		    }

		else  /* bytes_read is less than 0; presumably -1 */ {
		    /* If it's a system call interrupt, it's not an error. */
		    if (errno != EINTR)
		    	(*_XIOErrorFunction)(dpy);
		    }
	    	 }
}

/*
 * _XReadPad - Read bytes from the socket taking into account incomplete
 * reads.  If the number of bytes is not 0 mod 32, read additional pad
 * bytes. This routine may have to be reworked if int < long.
 */
_XReadPad (dpy, data, size)
    	register Display *dpy;	
	register char *data;
	register long size;
{
    	register long bytes_read;
	struct iovec iov[2];
	char pad[3];

	if (size == 0) return;
	iov[0].iov_len = (int)size;
	iov[0].iov_base = data;
	/* 
	 * The following hack is used to provide 32 bit long-word
	 * aligned padding.  The [1] vector is of length 0, 1, 2, or 3,
	 * whatever is needed.
	 */

	iov[1].iov_len = padlength[size & 3];
	iov[1].iov_base = pad;
	size += iov[1].iov_len;

	while ((bytes_read = ReadvFromServer (dpy->fd, iov, 2)) != size) {

	    if (bytes_read > 0) {
		size -= bytes_read;
	    	if ((iov[0].iov_len -= bytes_read) < 0) {
		    iov[1].iov_len += iov[0].iov_len;
		    iov[1].iov_base -= iov[0].iov_len;
		    iov[0].iov_len = 0;
		    }
	    	else
	    	    iov[0].iov_base += bytes_read;
	    	}

	    else if (errno == EWOULDBLOCK) {
		_XWaitForReadable(dpy);
		errno = 0;
	    }
	    else if (bytes_read == 0) {
		/* Read failed because of end of file! */
		errno = EPIPE;
		(*_XIOErrorFunction)(dpy);
		}
	    
	    else  /* bytes_read is less than 0; presumably -1 */ {
		/* If it's a system call interrupt, it's not an error. */
		if (errno != EINTR)
		    (*_XIOErrorFunction)(dpy);
		}
	    }

}

/*
 * _XSend - Flush the buffer and send the client data. 32 bit word aligned
 * transmission is used, if size is not 0 mod 4, extra bytes are transmitted.
 * This routine may have to be reworked if int < long;
 */
_XSend (dpy, data, size)
	register Display *dpy;
	char *data;
	register long size;
{
	register long len;
	struct iovec iov[3];
	static char pad[3] = {0, 0, 0};
           /* XText8 and XText16 require that the padding bytes be zero! */

	iov[0].iov_len = len = dpy->bufptr - dpy->buffer;
	iov[0].iov_base = dpy->bufptr = dpy->buffer;
	iov[1].iov_len = size;
	iov[1].iov_base = data;
	/* 
	 * The following hack is used to provide 32 bit long-word
	 * aligned padding.  The [2] vector is of length 0, 1, 2, or 3,
	 * whatever is needed.
	 */
	iov[2].iov_len = padlength[size & 3];
	iov[2].iov_base = pad;
	len += (size + 3) & ~3;
	/*
	 * Use while to allow for incremental writes.
	 */
	while ((size = WritevToServer(dpy->fd, iov, 3)) != len) {
	    if (size > 0) {
		len -= size;
		if ((iov[0].iov_len -= size) < 0) {
		    iov[1].iov_len += iov[0].iov_len;
		    iov[1].iov_base -= iov[0].iov_len;
		    iov[0].iov_len = 0;
		    if (iov[1].iov_len < 0) {
	    	    	iov[2].iov_len += iov[1].iov_len;
			iov[2].iov_base -= iov[1].iov_len;
			iov[1].iov_len = 0;
		    }
		}
		else {
			iov[0].iov_base += size;
		}
	    } else if (errno == EWOULDBLOCK) {
		_XWaitForWritable(dpy);
	    } else (*_XIOErrorFunction)(dpy);
	}
	dpy->last_req = (char *) & _dummy_request;
}

/*
 * _XAllocID - normal resource ID allocation routine.  A client
 * can roll his own and instatantiate it if he wants, but must
 * follow the rules.
 */
XID _XAllocID(dpy)
register Display *dpy;
{
   return (dpy->resource_base + (dpy->resource_id++ << dpy->resource_shift));
}

/*
 * _XReply - Wait for a reply packet and copy its contents into the
 * specified rep.  Mean while we must handle error and event packets that
 * we may encounter.
 */
Status _XReply (dpy, rep, extra, discard)
    register Display *dpy;
    register xReply *rep;
    int extra;		/* number of 32-bit words expected after the reply */
    Bool discard;	/* should I discard data followind "extra" words? */
{
    _XFlush(dpy);
    while (1) {
	_XRead(dpy, (char *)rep, (long)sizeof(xReply));
	switch ((int)rep->generic.type) {

	    case X_Reply:
	        /* Reply recieved. */
		if (extra == 0) {
		    if (discard && (rep->generic.length > 0))
		       /* unexpectedly long reply! */
		       _EatData (dpy, rep->generic.length);
		    return (1);
		    }
		if (extra == rep->generic.length) {
		    /* 
		     * Read the extra data into storage immediately following
		     * the GenericReply structure. 
		     */
		    _XRead (dpy, (char *) (rep+1), ((long)extra)<<2);
		    return (1);
		    }
		if (extra < rep->generic.length) {
		    /* Actual reply is longer than "extra" */
		    _XRead (dpy, (char *) (rep+1), ((long)extra)<<2);
		    if (discard)
		        _EatData (dpy, rep->generic.length - extra);
		    return (1);
		    }
		/* 
		 *if we get here, then extra > rep->generic.length--meaning we
		 * read a reply that's shorter than we expected.  This is an 
		 * error,  but we still need to figure out how to handle it...
		 */
		_XRead (dpy, (char *) (rep+1), (long) (rep->generic.length<<2));
		(*_XIOErrorFunction) (dpy);
		return (0);

    	    case X_Error:
	    	{
	        register _XExtension *ext;
		register Bool ret = False;
		int ret_code;
		xError *err = (xError *) rep;
		if (err->sequenceNumber == dpy->request)
			/* do not die on "no such font", "can't allocate",
			   "can't grab" failures */
			switch ((int)err->errorCode) {
			case BadName:
			    switch (err->majorCode)
				case X_OpenFont:
				case X_LookupColor:
				case X_AllocNamedColor:
				    return(0);
				break;
			case BadFont:
			    if (err->majorCode == X_QueryFont)
				return (0);
			    break;
			case BadAlloc:
			case BadAccess:
				return (0);
			/* 
			 * we better see if there is an extension who may
			 * want to suppress the error.
			 */
			default:
			    ext = dpy->ext_procs;
			    while (ext) {
				if (ext->error != NULL) 
				   ret = (*ext->error)
					(dpy, err, &ext->codes, &ret_code);
				ext = ext->next;
				}
			    if (ret) return (ret_code);
			    break;
			}
		_XError(dpy, err);
		if (err->sequenceNumber == dpy->request)
		    return(0);
		}
		break;
	    default:
		_XEnq(dpy, (xEvent *) rep);
		break;
	    }
	}
}   


/* Read and discard "n" 32-bit words. */

static _EatData (dpy, n)
    Display *dpy;
    unsigned long n;
    {
    unsigned int bufsize;
    char *buf;
    n <<= 2;  /* convert to number of bytes */
    buf = Xmalloc (bufsize = (n > 2048) ? 2048 : n);
    while (n) {
	long bytes_read = (n > bufsize) ? bufsize : n;
	_XRead (dpy, buf, bytes_read);
	n -= bytes_read;
	}
    Xfree (buf);
    }

/*
 * _XEnq - Place event packets on the display's queue.
 * note that no squishing of move events in V11, since there
 * is pointer motion hints....
 */
_XEnq (dpy, event)
	register Display *dpy;
	register xEvent *event;
{
	register _XQEvent *qelt;

/*NOSTRICT*/
	if (qelt = _qfree) {
		/* If _qfree is non-NULL do this, else malloc a new one. */
		_qfree = qelt->next;
	}
	else if ((qelt = 
	    (_XQEvent *) Xmalloc((unsigned)sizeof(_XQEvent))) == NULL) {
		/* Malloc call failed! */
		errno = ENOMEM;
		(*_XIOErrorFunction)(dpy);
	}
	qelt->next = NULL;
	/* go call through display to find proper event reformatter */
	(*dpy->event_vec[event->u.u.type & 0177])(dpy, &qelt->event, event);

	if (dpy->tail)	dpy->tail->next = qelt;
	else 		dpy->head = qelt;

	dpy->tail = qelt;
	dpy->qlen++;
}
/*
 * EventToWire in seperate file in that often not needed.
 */

/*ARGSUSED*/
_XUnknownWireEvent(dpy, re, event)
register Display *dpy;	/* pointer to display structure */
register XEvent *re;	/* pointer to where event should be reformatted */
register xEvent *event;	/* wire protocol event */
{
	(void) fprintf(stderr, 
	    "Xlib: unhandled wire event! event number = %d, display = %x\n.",
			event->u.u.type, dpy);
	exit(1);
}

/*ARGSUSED*/
_XUnknownNativeEvent(dpy, re, event)
register Display *dpy;	/* pointer to display structure */
register XEvent *re;	/* pointer to where event should be reformatted */
register xEvent *event;	/* wire protocol event */
{
	(void) fprintf(stderr, 
 	   "Xlib: unhandled native event! event number = %d, display = %x\n.",
			re->type, dpy);
	exit(1);
}
/*
 * reformat a wire event into an XEvent structure of the right type.
 */
_XWireToEvent(dpy, re, event)
register Display *dpy;	/* pointer to display structure */
register XEvent *re;	/* pointer to where event should be reformatted */
register xEvent *event;	/* wire protocol event */
{

	((XAnyEvent *)re)->display = dpy;	
	re->type = event->u.u.type;
	
	/* Ignore the leading bit of the event type since it is set when a
		client sends an event rather than the server. */

	switch (event-> u.u.type & 0177) {
	      case KeyPress:
	      case KeyRelease:
	        {
			register XKeyEvent *ev = (XKeyEvent*) re;
			ev->root 	= event->u.keyButtonPointer.root;
			ev->window 	= event->u.keyButtonPointer.event;
			ev->subwindow 	= event->u.keyButtonPointer.child;
			ev->time 	= event->u.keyButtonPointer.time;
			ev->x 		= event->u.keyButtonPointer.eventX;
			ev->y 		= event->u.keyButtonPointer.eventY;
			ev->x_root 	= event->u.keyButtonPointer.rootX;
			ev->y_root 	= event->u.keyButtonPointer.rootY;
			ev->state	= event->u.keyButtonPointer.state;
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->keycode 	= event->u.u.detail;
		}
	      	break;
	      case ButtonPress:
	      case ButtonRelease:
	        {
			register XButtonEvent *ev =  (XButtonEvent *) re;
			ev->root 	= event->u.keyButtonPointer.root;
			ev->window 	= event->u.keyButtonPointer.event;
			ev->subwindow 	= event->u.keyButtonPointer.child;
			ev->time 	= event->u.keyButtonPointer.time;
			ev->x 		= event->u.keyButtonPointer.eventX;
			ev->y 		= event->u.keyButtonPointer.eventY;
			ev->x_root 	= event->u.keyButtonPointer.rootX;
			ev->y_root 	= event->u.keyButtonPointer.rootY;
			ev->state	= event->u.keyButtonPointer.state;
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->button 	= event->u.u.detail;
		}
	        break;
	      case MotionNotify:
	        {
			register XMotionEvent *ev =   (XMotionEvent *)re;
			ev->root 	= event->u.keyButtonPointer.root;
			ev->window 	= event->u.keyButtonPointer.event;
			ev->subwindow 	= event->u.keyButtonPointer.child;
			ev->time 	= event->u.keyButtonPointer.time;
			ev->x 		= event->u.keyButtonPointer.eventX;
			ev->y 		= event->u.keyButtonPointer.eventY;
			ev->x_root 	= event->u.keyButtonPointer.rootX;
			ev->y_root 	= event->u.keyButtonPointer.rootY;
			ev->state	= event->u.keyButtonPointer.state;
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->is_hint 	= event->u.u.detail;
		}
	        break;
	      case EnterNotify:
	      case LeaveNotify:
		{
			register XCrossingEvent *ev   = (XCrossingEvent *) re;
			ev->root	= event->u.enterLeave.root;
			ev->window	= event->u.enterLeave.event;
			ev->subwindow	= event->u.enterLeave.child;
			ev->time	= event->u.enterLeave.time;
			ev->x		= event->u.enterLeave.eventX;
			ev->y		= event->u.enterLeave.eventY;
			ev->x_root	= event->u.enterLeave.rootX;
			ev->y_root	= event->u.enterLeave.rootY;
			ev->state	= event->u.enterLeave.state;
			ev->mode	= event->u.enterLeave.mode;
			ev->same_screen = (event->u.enterLeave.flags & 
				ELFlagSameScreen) && True;
			ev->focus	= (event->u.enterLeave.flags &
			  	ELFlagFocus) && True;
			ev->detail	= event->u.u.detail;
		}
		  break;
	      case FocusIn:
	      case FocusOut:
		{
			register XFocusChangeEvent *ev = (XFocusChangeEvent *) re;
			ev->window 	= event->u.focus.window;
			ev->mode	= event->u.focus.mode;
			ev->detail	= event->u.u.detail;
		}
		  break;
	      case KeymapNotify:
		{
			register XKeymapEvent *ev = (XKeymapEvent *) re;
			ev->window	= dpy->current;
			bcopy ((char *)((xKeymapEvent *) event)->map,
			       &ev->key_vector[1], 
			       sizeof (((xKeymapEvent *) event)->map));
		}
		break;
	      case Expose:
		{
			register XExposeEvent *ev = (XExposeEvent *) re;
			ev->window	= event->u.expose.window;
			ev->x		= event->u.expose.x;
			ev->y		= event->u.expose.y;
			ev->width	= event->u.expose.width;
			ev->height	= event->u.expose.height;
			ev->count	= event->u.expose.count;
		}
		break;
	      case GraphicsExpose:
		{
		    register XGraphicsExposeEvent *ev =
			(XGraphicsExposeEvent *) re;
		    ev->drawable	= event->u.graphicsExposure.drawable;
		    ev->x		= event->u.graphicsExposure.x;
		    ev->y		= event->u.graphicsExposure.y;
		    ev->width		= event->u.graphicsExposure.width;
		    ev->height		= event->u.graphicsExposure.height;
		    ev->count		= event->u.graphicsExposure.count;
		    ev->major_code	= event->u.graphicsExposure.majorEvent;
		    ev->minor_code	= event->u.graphicsExposure.minorEvent;
		}
		break;
	      case NoExpose:
		{
		    register XNoExposeEvent *ev = (XNoExposeEvent *) re;
		    ev->drawable	= event->u.noExposure.drawable;
		    ev->major_code	= event->u.noExposure.majorEvent;
		    ev->minor_code	= event->u.noExposure.minorEvent;
		}
		break;
	      case VisibilityNotify:
		{
		    register XVisibilityEvent *ev = (XVisibilityEvent *) re;
		    ev->window		= event->u.visibility.window;
		    ev->state		= event->u.visibility.state;
		}
		break;
	      case CreateNotify:
		{
		    register XCreateWindowEvent *ev =
			 (XCreateWindowEvent *) re;
		    ev->window		= event->u.createNotify.window;
		    ev->parent		= event->u.createNotify.parent;
		    ev->x		= event->u.createNotify.x;
		    ev->y		= event->u.createNotify.y;
		    ev->width		= event->u.createNotify.width;
		    ev->height		= event->u.createNotify.height;
		    ev->border_width	= event->u.createNotify.borderWidth;
		    ev->override_redirect	= event->u.createNotify.override;
		}
		break;
	      case DestroyNotify:
		{
		    register XDestroyWindowEvent *ev =
				(XDestroyWindowEvent *) re;
		    ev->window		= event->u.destroyNotify.window;
		    ev->event		= event->u.destroyNotify.event;
		}
		break;
	      case UnmapNotify:
		{
		    register XUnmapEvent *ev = (XUnmapEvent *) re;
		    ev->window		= event->u.unmapNotify.window;
		    ev->event		= event->u.unmapNotify.event;
		    ev->from_configure	= event->u.unmapNotify.fromConfigure;
		}
		break;
	      case MapNotify:
		{
		    register XMapEvent *ev = (XMapEvent *) re;
		    ev->window		= event->u.mapNotify.window;
		    ev->event		= event->u.mapNotify.event;
		    ev->override_redirect	= event->u.mapNotify.override;
		}
		break;
	      case MapRequest:
		{
		    register XMapRequestEvent *ev = (XMapRequestEvent *) re;
		    ev->window		= event->u.mapRequest.window;
		    ev->parent		= event->u.mapRequest.parent;
		}
		break;
	      case ReparentNotify:
		{
		    register XReparentEvent *ev = (XReparentEvent *) re;
		    ev->event		= event->u.reparent.event;
		    ev->window		= event->u.reparent.window;
		    ev->parent		= event->u.reparent.parent;
		    ev->x		= event->u.reparent.x;
		    ev->y		= event->u.reparent.y;
		    ev->override_redirect	= event->u.reparent.override;
		}
		break;
	      case ConfigureNotify:
		{
		    register XConfigureEvent *ev = (XConfigureEvent *) re;
		    ev->event	= event->u.configureNotify.event;
		    ev->window	= event->u.configureNotify.window;
		    ev->above	= event->u.configureNotify.aboveSibling;
		    ev->x	= event->u.configureNotify.x;
		    ev->y	= event->u.configureNotify.y;
		    ev->width	= event->u.configureNotify.width;
		    ev->height	= event->u.configureNotify.height;
		    ev->border_width  = event->u.configureNotify.borderWidth;
		    ev->override_redirect = event->u.configureNotify.override;
		}
		break;
	      case ConfigureRequest:
		{
		    register XConfigureRequestEvent *ev =
		        (XConfigureRequestEvent *) re;
		    ev->window		= event->u.configureRequest.window;
		    ev->parent		= event->u.configureRequest.parent;
		    ev->above		= event->u.configureRequest.sibling;
		    ev->x		= event->u.configureRequest.x;
		    ev->y		= event->u.configureRequest.y;
		    ev->width		= event->u.configureRequest.width;
		    ev->height		= event->u.configureRequest.height;
		    ev->border_width	= event->u.configureRequest.borderWidth;
		    ev->value_mask	= event->u.configureRequest.valueMask;
		    ev->detail  	= event->u.u.detail;
		}
		break;
	      case GravityNotify:
		{
		    register XGravityEvent *ev = (XGravityEvent *) re;
		    ev->window		= event->u.gravity.window;
		    ev->event		= event->u.gravity.event;
		    ev->x		= event->u.gravity.x;
		    ev->y		= event->u.gravity.y;
		}
		break;
	      case ResizeRequest:
		{
		    register XResizeRequestEvent *ev =
			(XResizeRequestEvent *) re;
		    ev->window		= event->u.resizeRequest.window;
		    ev->width		= event->u.resizeRequest.width;
		    ev->height		= event->u.resizeRequest.height;
		}
		break;
	      case CirculateNotify:
		{
		    register XCirculateEvent *ev = (XCirculateEvent *) re;
		    ev->window		= event->u.circulate.window;
		    ev->event		= event->u.circulate.event;
		    ev->place		= event->u.circulate.place;
		}
		break;
	      case CirculateRequest:
		{
		    register XCirculateRequestEvent *ev =
		        (XCirculateRequestEvent *) re;
		    ev->window		= event->u.circulate.window;
		    ev->parent		= event->u.circulate.event;
		    ev->place		= event->u.circulate.place;
		}
		break;
	      case PropertyNotify:
		{
		    register XPropertyEvent *ev = (XPropertyEvent *) re;
		    ev->window		= event->u.property.window;
		    ev->atom		= event->u.property.atom;
		    ev->time		= event->u.property.time;
		    ev->state		= event->u.property.state;
		}
		break;
	      case SelectionClear:
		{
		    register XSelectionClearEvent *ev =
			 (XSelectionClearEvent *) re;
		    ev->window		= event->u.selectionClear.window;
		    ev->selection	= event->u.selectionClear.atom;
		    ev->time		= event->u.selectionClear.time;
		}
		break;
	      case SelectionRequest:
		{
		    register XSelectionRequestEvent *ev =
		        (XSelectionRequestEvent *) re;
		    ev->owner		= event->u.selectionRequest.owner;
		    ev->requestor	= event->u.selectionRequest.requestor;
		    ev->selection	= event->u.selectionRequest.selection;
		    ev->target		= event->u.selectionRequest.target;
		    ev->property	= event->u.selectionRequest.property;
		    ev->time		= event->u.selectionRequest.time;
		}
		break;
	      case SelectionNotify:
		{
		    register XSelectionEvent *ev = (XSelectionEvent *) re;
		    ev->requestor	= event->u.selectionNotify.requestor;
		    ev->selection	= event->u.selectionNotify.selection;
		    ev->target		= event->u.selectionNotify.target;
		    ev->property	= event->u.selectionNotify.property;
		    ev->time		= event->u.selectionNotify.time;
		}
		break;
	      case ColormapNotify:
		{
		    register XColormapEvent *ev = (XColormapEvent *) re;
		    ev->window		= event->u.colormap.window;
		    ev->colormap	= event->u.colormap.colormap;
		    ev->new		= event->u.colormap.new;
		    ev->state		= event->u.colormap.state;
	        }
		break;
	      case ClientMessage:
		{
		   register int i;
		   register XClientMessageEvent *ev 
		   			= (XClientMessageEvent *) re;
		   ev->window		= event->u.clientMessage.window;
		   ev->format		= event->u.u.detail;
		   switch (ev->format) {
			case 8:	
			   ev->message_type = event->u.clientMessage.u.b.type;
			   for (i = 0; i < 20; i++) 	
			     ev->data.b[i] = event->u.clientMessage.u.b.bytes[i];
			   break;
			case 16:
			   ev->message_type = event->u.clientMessage.u.s.type;
			   ev->data.s[0] = event->u.clientMessage.u.s.shorts0;
			   ev->data.s[1] = event->u.clientMessage.u.s.shorts1;
			   ev->data.s[2] = event->u.clientMessage.u.s.shorts2;
			   ev->data.s[3] = event->u.clientMessage.u.s.shorts3;
			   ev->data.s[4] = event->u.clientMessage.u.s.shorts4;
			   ev->data.s[5] = event->u.clientMessage.u.s.shorts5;
			   ev->data.s[6] = event->u.clientMessage.u.s.shorts6;
			   ev->data.s[7] = event->u.clientMessage.u.s.shorts7;
			   ev->data.s[8] = event->u.clientMessage.u.s.shorts8;
			   ev->data.s[9] = event->u.clientMessage.u.s.shorts9;
			   break;
			case 32:
			   ev->message_type = event->u.clientMessage.u.l.type;
			   ev->data.l[0] = event->u.clientMessage.u.l.longs0;
			   ev->data.l[1] = event->u.clientMessage.u.l.longs1;
			   ev->data.l[2] = event->u.clientMessage.u.l.longs2;
			   ev->data.l[3] = event->u.clientMessage.u.l.longs3;
			   ev->data.l[4] = event->u.clientMessage.u.l.longs4;
			   break;
			default: /* XXX should never occur */
				break;
		    }
	        }
		break;
	      case MappingNotify:
		{
		   register XMappingEvent *ev = (XMappingEvent *)re;
		   ev->first_keycode 	= event->u.mappingNotify.firstKeyCode;
		   ev->request 		= event->u.mappingNotify.request;
		   ev->count 		= event->u.mappingNotify.count;
		}
		break;
	      default:
		/* XXX should do something about unknown event here */
		break;
	}
}


/*
 * _XIOError - Default fatal system error reporting routine.  Called when
 * an X internal system error is encountered.
 */
_XIOError (dpy)
	Display *dpy;
{
  	if (errno == EPIPE) {
	    (void) fprintf (stderr, 
			"Connection # %d to server broken.\n", dpy->fd);
	  }
	perror("XIO");
	exit(1);
}

/*
 * _XError - Default non-fatal error reporting routine.  Called when an
 * X_Error packet is encountered in the input stream.
 */
int _XError (dpy, rep)
    Display *dpy;
    xError *rep;
{
    XErrorEvent event;
    /* 
     * X_Error packet encountered!  We need to unpack the error before
     * giving it to the user.
     */

    event.display = dpy;
    event.type = X_Error;
    event.serial = rep->sequenceNumber;
    event.resourceid = rep->resourceID;
    event.error_code = rep->errorCode;
    event.request_code = rep->majorCode;
    event.minor_code = rep->minorCode;
    if (_XErrorFunction != NULL) {
      	return ((*_XErrorFunction)(dpy, &event));
      }
    exit(1);
    return(0);			/* stupid lint */
}
    
int _XDefaultError(dpy, event)
	Display *dpy;
	XErrorEvent *event;
{
    char buffer[BUFSIZ];
    char mesg[BUFSIZ];
    char number[32];
    char *mtype = "XlibMessage";
    XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
    XGetErrorDatabaseText(dpy, mtype, "XError", "X Error", mesg, BUFSIZ);
    (void) fprintf(stderr, "%s: %s\n  ", mesg, buffer);
    XGetErrorDatabaseText(dpy, mtype, "MajorCode", "Request Major code %d", 
	mesg, BUFSIZ);
    (void) fprintf(stderr, mesg, event->request_code);
    sprintf(number, "%d", event->request_code);
    XGetErrorDatabaseText(dpy, "XRequest", number, "", 	buffer, BUFSIZ);
    (void) fprintf(stderr, " %s", buffer);
    fputs("\n  ", stderr);
    XGetErrorDatabaseText(dpy, mtype, "MinorCode", "Request Minor code", 
	mesg, BUFSIZ);
    (void) fprintf(stderr, mesg, event->minor_code);
    fputs("\n  ", stderr);
    XGetErrorDatabaseText(dpy, mtype, "ResourceID", "ResourceID 0x%x",
	mesg, BUFSIZ);
    (void) fprintf(stderr, mesg, event->resourceid);
    fputs("\n  ", stderr);
    XGetErrorDatabaseText(dpy, mtype, "ErrorSerial", "Error Serial #%d", 
	mesg, BUFSIZ);
    (void) fprintf(stderr, mesg, event->serial);
    fputs("\n  ", stderr);
    XGetErrorDatabaseText(dpy, mtype, "CurrentSerial", "Current Serial #%d",
	mesg, BUFSIZ);
    (void) fprintf(stderr, mesg, dpy->request);
    fputs("\n  ", stderr);
    if (event->error_code == BadImplementation) return 0;
    exit(1);
    return (0);			/* stupid lint */
}

int (*_XIOErrorFunction)() = _XIOError;
int (*_XErrorFunction)() = _XDefaultError;

/*
 * This routine can be used to (cheaply) get some memory within a single
 * Xlib routine for scratch space.  It is reallocated from the same place
 * each time, unless the library needs a large scratch space.
 */
char *_XAllocScratch (dpy, nbytes)
	register Display *dpy;
	unsigned long nbytes;
{
	if (nbytes > dpy->scratch_length) {
	    if (dpy->scratch_buffer != NULL) Xfree (dpy->scratch_buffer);
	    return( dpy->scratch_length = nbytes, 
	    dpy->scratch_buffer = Xmalloc ((unsigned)nbytes) );
	}
	return (dpy->scratch_buffer);
}

/*
 * Given a visual id, find the visual structure for this id on this display.
 */
Visual *_XVIDtoVisual (dpy, id)
	Display *dpy;
	VisualID id;
{
	register int i, j, k;
	register Screen *sp;
	register Depth *dp;
	register Visual *vp;
	for (i = 0; i < dpy->nscreens; i++) {
		sp = &dpy->screens[i];
		for (j = 0; j < sp->ndepths; j++) {
			dp = &sp->depths[j];
			for (k = 0; k < dp->nvisuals; k++) {
				vp = &dp->visuals[k];
				if (vp->visualid == id) return (vp);
			}
		}
	}
	return (NULL);
}

XFree (data)
	char *data;
{
	Xfree (data);
}

#ifdef BIGSHORTS
UnpackShorts(from, to, bytes)
	ushort_p *from;
	short *to;
	unsigned bytes;
{
	unsigned i;
	for (i = 0; i < (bytes/psizeof(short)); i++)
		if (i&1)
			to[i] = from[i>>1].right;
		else
			to[i] = from[i>>1].left;
}

char packbuffer[1000];
PackData(dpy, data, len)
    register Display *dpy;
    short *data;
    unsigned len;
{
	if (dpy->bufptr + len < dpy->bufmax) {
		PackShorts(data, dpy->bufptr, len);
		dpy->bufptr += (len + 3) & ~3;
	} else {
		PackShorts(data, packbuffer, len);
		_XSend(dpy, packbuffer, len);
	}
}

PackShorts(from, to, bytes)
	short *from;
	char *to;
	unsigned bytes;
{
	unsigned i, n, offset;
	ushort_p *uto;

	uto = (ushort_p *)to;
	offset = ((int)to & 2) >> 1; /* lost 2 bits of pointer */
	n = (bytes / 2) + offset;
	for (i = offset; i < n; i++) {
		if (i&1)
			uto[i>>1].right = from[i-offset];
		else
			uto[i>>1].left = from[i-offset];
	}
}
#endif
