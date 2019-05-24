#include "copyright.h"

/* $Header: Xlibint.h,v 11.40 87/06/24 11:32:42 swick Exp $ */
/* Copyright 1984, 1985, 1987  Massachusetts Institute of Technology */

/*
 *	XlibInternal.h - Header definition and support file for the internal
 *	support routines (XlibInternal) used by the C subroutine interface
 *	library (Xlib) to the X Window System.
 *
 */
#ifndef NEED_EVENTS
#define _XEVENT_
#endif
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include "Xlibos.h"
#include <errno.h>

#ifndef NULL
#define NULL 0
#endif
#define LOCKED 1
#define UNLOCKED 0

extern int errno;			/* Internal system error number. */
extern void bcopy();

extern (*_XIOErrorFunction)();		/* X system error reporting routine. */
extern (*_XErrorFunction)();		/* X_Error event reporting routine. */
extern char *_XAllocScratch();		/* fast memory allocator */
extern Visual *_XVIDtoVisual();		/* given visual id, find structure */

#define BUFSIZE 2048			/* X output buffer size. */
#define EPERBATCH 8			/* when batching, how many elements */
#define CURSORFONT "cursor"		/* standard cursor fonts */

/*
 * X Protocol packetizing macros.
 */


/*
 * GetReq - Get the next avilable X request packet in the buffer and
 * return it. 
 *
 * "name" is the name of the request, e.g. CreatePixmap, OpenFont, etc.
 * "req" is the name of the request pointer.
 *
 */

#define GetReq(name, req) \
	if ((dpy->bufptr + sizeof(x/**/name/**/Req)) > dpy->bufmax)\
		_XFlush(dpy);\
	req = (x/**/name/**/Req *)(dpy->last_req = dpy->bufptr);\
	req->reqType = X_/**/name;\
	req->length = (sizeof(x/**/name/**/Req))>>2;\
	dpy->bufptr += sizeof(x/**/name/**/Req);\
	dpy->request++

/* GetReqExtra is the same as GetReq, but allocates "n" additional
   bytes after the request. "n" must be a multiple of 4!  */


#define GetReqExtra(name, n, req) \
	if ((dpy->bufptr + sizeof(x/**/name/**/Req) + n) > dpy->bufmax)\
		_XFlush(dpy);\
	req = (x/**/name/**/Req *)(dpy->last_req = dpy->bufptr);\
	req->reqType = X_/**/name;\
	req->length = (sizeof(x/**/name/**/Req) + n)>>2;\
	dpy->bufptr += sizeof(x/**/name/**/Req) + n;\
	dpy->request++

/*
 * GetResReq is for those requests that have a resource ID 
 * (Window, Pixmap, GContext, etc.) as their single argument.
 * "rid" is the name of the resource. 
 */

#define GetResReq(name, rid, req) \
	if ((dpy->bufptr + sizeof(xResourceReq)) > dpy->bufmax)\
	    _XFlush(dpy);\
	req = (xResourceReq *) (dpy->last_req = dpy->bufptr);\
	req->reqType = X_/**/name;\
	req->length = 2;\
	req->id = (rid);\
	dpy->bufptr += sizeof(xResourceReq);\
	dpy->request++

/*
 * GetEmptyReq is for those requests that have no arguments
 * at all. 
 */

#define GetEmptyReq(name, req) \
	if ((dpy->bufptr + sizeof(xReq)) > dpy->bufmax)\
	    _XFlush(dpy);\
	req = (xReq *) (dpy->last_req = dpy->bufptr);\
	req->reqType = X_/**/name;\
	req->length = 1;\
	dpy->bufptr += sizeof(xReq);\
	dpy->request++

#define SyncHandle() \
	if (dpy->synchandler) (*dpy->synchandler)(dpy)

#define FlushGC(dpy, gc) \
	if ((gc)->dirty) _XFlushGCCache((dpy), (gc))
/*
 * Data - Place data in the buffer and pad the end to provide
 * 32 bit word alignment.  Transmit if the buffer fills.
 *
 * "dpy" is a pointer to a Display.
 * "data" is a pinter to a data buffer.
 * "len" is the length of the data buffer.
 * we can presume buffer less than 2^16 bytes, so bcopy can be used safely.
 */
#define Data(dpy, data, len) \
	if (dpy->bufptr + (len) <= dpy->bufmax) {\
		bcopy(data, dpy->bufptr, (int)len);\
		dpy->bufptr += ((len) + 3) & ~3;\
	} else\
		_XSend(dpy, data, len)


/* Allocate bytes from the buffer.  No padding is done, so if
 * the length is not a multiple of 4, the caller must be
 * careful to leave the buffer aligned after sending the
 * current request.
 *
 * "type" is the type of the pointer being assigned to.
 * "ptr" is the pointer being assigned to.
 * "n" is the number of bytes to allocate.
 *
 * Example: 
 *    xTextElt *elt;
 *    BufAlloc (xTextElt *, elt, nbytes)
 */

#define BufAlloc(type, ptr, n) \
    if (dpy->bufptr + (n) > dpy->bufmax) \
        _XFlush (dpy); \
    ptr = (type) dpy->bufptr; \
    dpy->bufptr += (n);

#ifndef BIGSHORTS
#define PackData(dpy, data, len) Data(dpy, data, len)
#define PackShorts(f, t, n)  bcopy(f, t, n)
#endif

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define	CI_NONEXISTCHAR	0x4000	/* required because QueryFont represents
				   a non-existant character with zero-value
				   metrics, but requires drivers to output
				   the default char in their place. */
