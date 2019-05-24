/*
 *	rcs_id[] = "$Header: macros.h,v 1.14 87/09/11 08:18:08 toddb Exp $";
 */

/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documentation,
 * and that the name of Digital Equipment Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 */

#define FILEPTR		FILE *

#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))

#define abs(x)		((x) < 0 ? (-(x)) : (x))

#define QXOpenDisplay	XOpenDisplay

#ifdef X11
#define DISPLAY		theDisplay,
#define QDefaultRootWindow	DefaultRootWindow
#define QXDestroyWindow	XDestroyWindow
#define QXFlush		XFlush
#define QXMapWindow	XMapWindow
#define QXMoveWindow	XMoveWindow
#define QXNextEvent	XtNextEvent
#define QXPeekEvent	XPeekEvent
#define QXPending	XPending
#define QXPutBackEvent	XPutBackEvent
#define QXStoreName	XStoreName
#define QXSync		XSync
#define QXUnmapWindow	XUnmapWindow
#define QConnectionNumber	ConnectionNumber
#define QXSetInputFocus	XSetInputFocus
#endif X11

#ifdef X10
#define DISPLAY
#define QDefaultRootWindow(dpy)		RootWindow
#define QXDestroyWindow(dpy, win)	XDestroyWindow(win)
#define QXFlush(dpy)			XFlush()
#define QXMapWindow(dpy, win)		XMapWindow(win)
#define QXMoveWindow(dpy, win, x, y)	XMoveWindow(win, x, y)
#define QXUnmapWindow(dpy, win)		XUnmapWindow(win)
#define QXNextEvent(dpy, event)		XNextEvent(event)
#define QXPeekEvent(dpy, event)		XPeekEvent(event)
#define QXPending(dpy)			XPending()
#define QXPutBackEvent(dpy, event)	XPutBackEvent(event)
#define QXStoreName(dpy, win, name)	XStoreName(win, name)
#define QXSync(dpy, value)		XSync(value)
#define QConnectionNumber(dpy)			dpyno()
#define KeyPress			KeyPressed
#define KeyRelease			KeyReleased
#define ButtonPress			ButtonPressed
#define ButtonRelease			ButtonReleased
#define QXSetInputFocus(d, w, a, b)

#endif X10



#ifdef X10
typedef int Position;
typedef unsigned int Dimension;
#define XtsdLeft sdLEFT
#define XtsdRight sdRIGHT

#define XtstPositions stPOSITIONS
#define XtstWhiteSpace stWHITESPACE
#define XtstEOL stEOL
#define XtstParagraph stPARAGRAHP
#define XtstFile stFILE

#define XtgeometryGetWindowBox geometryGETWINDOWBOX
#define XtGeometryRequest int

#define XrmRBoolean		XtRBoolean
#define XrmRString		XtRString
#define XrmRInt			XtRInt
#define XrmResourceDataBase	ResourceDataBase
#define XrmNameList		XtNameList
#define XrmClassList		XtClassList
#define XrmGetDataBase		XtGetDataBase
#define XrmMergeDataBases	XtMergeDataBases
#define XrmSetCurrentDataBase	XtSetCurrentDataBase
#define XrmFreeNameList		XtFreeNameList
#define XrmFreeClassList	XtFreeClassList
#endif
