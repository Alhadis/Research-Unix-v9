/* $Header: Intrinsic.h,v 1.2 87/07/13 09:27:06 toddb Exp $ */
/*
 *	sccsid:	%W%	%G%
 */

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef _Xtintrinsic_h
#define _Xtintrinsic_h

#include <X11/Xresource.h>
#include <X11/Xutil.h>

/****************************************************************
 ****************************************************************
 ***                                                          ***
 ***                                                          ***
 ***                   X Toolkit Intrinsics                   ***
 ***                                                          ***
 ***                                                          ***
 ****************************************************************
 ****************************************************************/


/****************************************************************
 *
 * System Dependent Definitions
 *
 *
 * The typedef for XtArgVal should be chosen such that
 *      sizeof (XtArgVal) == max (sizeof(caddr_t), sizeof(long))
 *
 * ArgLists rely heavily on the above typedef.
 *
 ****************************************************************/

typedef char *XtArgVal;



/****************************************************************
 *
 * Miscellaneous definitions
 *
 ****************************************************************/

#include	<sys/types.h>

#ifndef NULL
#define NULL 0
#endif

#define Boolean int
#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#define XtNumber(arr)			(sizeof(arr) / sizeof(arr[0]))

typedef char *String;

typedef unsigned long   GCMask;     /* Mask of values that are used by widget*/
typedef unsigned long   Pixel;	    /* Index into colormap	      */
typedef int		Position;   /* Offset from 0 coordinate	      */
typedef unsigned int	Dimension;  /* Size in pixels		      */


/****************************************************************
 *
 * Error codes
 *
 ****************************************************************/

typedef	int XtStatus;

#define XtSUCCESS 0	/* No error. */

extern int (*XtErrorFunction)();
extern int XtreferenceCount;


#define XtNOMEM	  1		/* Out of memory. */
#define XtFOPEN   3		/* fopen failed. */
#define XtNOTWINDOW 4		/* Given window id is invalid. */


/****************************************************************
 *
 * Toolkit initialization
 *
 ****************************************************************/

extern void    XtInitialize();

/****************************************************************
 *
 * Memory Allocation
 *
 ****************************************************************/

#ifndef _Alloc_c_

extern char *XtMalloc(); /* size */
    /* unsigned size; */

extern char *XtCalloc(); /* num, size */
    /* unsigned num, size; */

extern char *XtRealloc(); /* ptr, num */
    /* char     *ptr; */
    /* unsigned num; */

extern void XtFree(); /* ptr */
    /* char *ptr; */

#endif




/****************************************************************
 *
 * Arg lists
 *
 ****************************************************************/

typedef struct {
    XrmAtom	name;
    XtArgVal	value;
} Arg, *ArgList;

#define XtSetArg(arg, n, d) \
    ( (arg).name = (n), (arg).value = (XtArgVal)(d) )

extern ArgList XtMergeArgLists(); /* args1, argCount1, args2, argCount2 */
    /* ArgList args1;       */
    /* int     argCount1;   */
    /* ArgList args2;       */
    /* int     argCount2;   */



/****************************************************************
 *
 * Cursor Management
 *
 ****************************************************************/

extern Cursor XtGetCursor(); /* num */
    /* int num; */


/****************************************************************
 *
 * Event Management
 *
 ****************************************************************/

typedef enum {
    XteventHandled,	/* Event dispatched and handled			    */
    XteventNotHandled,	/* Event dispatched but not handled by event proc   */
    XteventNoHandler,	/* No event handler proc found			    */
} XtEventReturnCode;

#define XtINPUT_READ	(1)
#define XtINPUT_WRITE	(2)
#define XtINPUT_EXCEPT	(4)

/* These are hand-generated atoms for use in ClientMessage events
 * They are never sent to the server. They will not conflict with
 * any server-generated atoms because one of the 3 most significant
 * bits is set. We should consider getting real atoms from the server
 * at runtime 
 */

#define XtHasInput (0x80000001)
/* XClientEvent.data.l[0] is source
 * XClientEvent.data.l[1] is condition
 */

#define XtTimerExpired (0x80000002)
/* XClientEvent.data.l[0] is cookie
 */

typedef XtEventReturnCode (*XtEventHandler)(); /* event, data */
    /* XEvent  *event;  */
    /* caddr_t data;    */

extern void XtSetEventHandler(); /* w, proc, eventMask, data */
    /* Window		w;	    */
    /* XtEventHandler   proc;       */
    /* unsigned long    eventMask;  */
    /* caddr_t		data;       */

extern void XtSetGlobalEventHandler(); /* dpy, proc, eventMask, data */
    /* Display		*dpy;	    */
    /* XtEventHandler   proc;       */
    /* unsigned long    eventMask;  */
    /* caddr_t		data;       */

extern void XtDeleteEventHandler(); /* w, proc */
    /* Window         w;       */
    /* XtEventHandler proc;    */

extern void XtClearEventHandlers(); /* window */
    /* Window window; */

extern void XtMakeMaster(); /* w */
    /* Window w; */

extern XtEventReturnCode XtDispatchEvent(); /* event */
    /* XEvent	*event; */

/****************************************************************
 *
 * Event Gathering Routines
 *
 ****************************************************************/

extern void XtSetTimeOut(); /* wID, cookie, interval */
    /* Window wID; */
    /* caddr_t cookie; */
    /* int interval; */

extern int XtGetTimeOut(); /* wID, cookie */
    /* Window wID; */
    /* caddr_t cookie; */

extern int XtClearTimeOut(); /* wID, cookie */
    /* Window wID; */
    /* caddr_t cookie; */

extern void XtAddInput(); /* wID, source, condition */
    /* Window wID; */
    /* int source; */
    /* int condition; */

extern void XtRemoveInput(); /* wID, source, condition */
    /* Window wID; */
    /* int source; */
    /* int condition; */

extern void XtNextEvent(); /* event */
    /* XtEvent *event; */

extern XtPeekEvent(); /* event */
    /* XtEvent *event; */


/****************************************************************
 *
 * Geometry Management
 *
 ****************************************************************/

typedef enum {
    XtgeometryMove,		/* Move window to wb.x, wb.y		*/
    XtgeometryResize,		/* Resize window to wb.width, wb.height */
    XtgeometryTop,		/* Move window to top of stack		*/
    XtgeometryBottom,		/* Move window to bottom of stack       */
    XtgeometryGetWindowBox	/* Return window position, dimensions   */
} XtGeometryRequest;

typedef struct {
    Position x, y;
    Dimension width, height, borderWidth;
} WindowBox, *WindowBoxPtr;

typedef enum  {
    XtgeometryYes,        /* Request accepted. */
    XtgeometryNo,         /* Request denied. */
    XtgeometryAlmost,     /* Request denied, but willing to take replyBox. */
    XtgeometryNoManager   /* Request denied: couldn't find geometry manager */
} XtGeometryReturnCode;

typedef XtGeometryReturnCode (*XtGeometryHandler)();
    /* dpy, w, request, requestBox, replyBox */
    /* Display		    *dpy;		    */
    /* Window		    w;			    */
    /* XtGeometryRequest    request;		    */
    /* WindowBox	    *requestBox;	    */
    /* WindowBox	    *replyBox;    /* RETURN */

extern XtStatus XtSetGeometryHandler();  /* dpy, w, proc */
    /* Display		*dpy;	*/
    /* Window		 w;     */
    /* XtGeometryHandler proc;  */

extern XtStatus XtGetGeometryHandler(); /* dpy, w, proc */
    /* Display		*dpy;		    */
    /* Window		 w;		    */
    /* XtGeometryHandler *proc;   /* RETURN */

extern XtStatus XtClearGeometryHandler();  /* w */
    /* Window w; */

extern XtGeometryReturnCode XtMakeGeometryRequest();
    /* dpy, window, request, requestBox, replyBox */
    /* Display		*dpy;			*/
    /* Window		 window;		*/
    /* XtGeometryRequest request;		*/
    /* WindowBox	 *requestBox;		*/
    /* WindowBox	 *replyBox;   /* RETURN */

typedef struct {
    Window      w;
    WindowBox   wb;
} WindowLug, *WindowLugPtr;


/****************************************************************
 *
 * Graphic Context Management
 *****************************************************************/

extern GC XtGetGC(); /* widgetKind, drawable, valueMask, values */
    /* XContext widgetKind; */
    /* Drawable  d; */
    /* int       valueMask; */
    /* XGCValues *values; */


/****************************************************************
 *
 * Messages
 *
 ****************************************************************/

extern XtEventReturnCode XtSendEvent(); /* w, type */
    /* Window w; */
    /* unsigned long type; */

extern XtEventReturnCode XtSendDestroyNotify(); /* w */
    /* Window w; */

extern XtEventReturnCode XtSendExpose(); /* w */
    /* Window w; */

extern XtEventReturnCode XtSendConfigureNotify(); /* w, box */
    /* Window    w;     */
    /* WindowBox box;   */

extern XtEventReturnCode XtSendMessage(); /* w, message, args, argCount */
    /* Window       w;		*/
    /* XtMessage    message;    */
    /* ArgList      args;       */
    /* int	    argCount;   */

extern XtStatus XtGetWindowSize(); /* window, width, height, borderWidth */
    /* Window window;				      */
    /* int *width, *height, *borderWidth;   /* RETURN */


/****************************************************************
 *
 * Resources
 *
 ****************************************************************/

typedef struct _Resource {
    XrmAtom		name;		/* Resource name		    */
    XrmAtom		class;		/* Resource class		    */
    XrmAtom		type;		/* Representation type desired      */
    unsigned int	size;		/* Size in bytes of representation  */
    caddr_t		addr;		/* Where to put resource value      */
    caddr_t		defaultaddr;    /* Default resource value (or addr) */
} Resource, *ResourceList;


extern void XtGetResources();
    /* resources, resourceCount, args, argCount,
       parent, widgetName, widgetClass, names, classes */
    /* ResourceList resources;		*/
    /* int	    resourceCount;      */
    /* ArgList	    args;		*/
    /* int	    argCount;		*/
    /* Window	    parent;		*/
    /* XrmAtom	    widgetName;		*/
    /* XrmAtom	    widgetClass;	*/
    /* XrmNameList   *names;   /* RETURN */
    /* XrmClassList  *classes; /* RETURN */

extern void XtSetNameAndClass(); /* dpy, w, names, classes */
    /* Display	    *dpy;		*/
    /* Window       w;			*/
    /* XrmNameList   names;		*/
    /* XrmClassList  classes;		*/

extern void XtGetValues(); /* resources, resourceCount, args, argCount */
    /* ResourceList	resources;      */
    /* int		resourceCount;  */
    /* ArgList		args;		*/
    /* int		argCount;       */

extern void XtSetValues(); /* resources, resourceCount, args, argCount */
    /* ResourceList	resources;      */
    /* int		resourceCount;  */
    /* ArgList		args;		*/
    /* int		argCount;       */

extern int XtDefaultFGPixel, XtDefaultBGPixel;




/****************************************************************
 *
 * Translation Management
 *
 ****************************************************************/

typedef caddr_t XtEventsPtr;

typedef struct {
    char    *string;
    caddr_t value;
} XtActionsRec, *XtActionsPtr;

/* Different classes of action tokens */

typedef enum {XttokenChar, XttokenString, XttokenAction} TokenType;

/* List of tokens. */

typedef XrmQuark XtAction;
#define XtAtomToAction(atom)    ((XtAction) XrmAtomToQuark(atom))

typedef struct _XtActionTokenRec {
    TokenType type;
    union {
	char     c;
	char     *str;
	XtAction action;
    } value;
    struct _XtActionTokenRec *next;
} XtActionTokenRec, *XtActionTokenPtr;

typedef struct _TranslationRec *TranslationPtr;

extern XtEventsPtr XtSetActionBindings();
			/* eventTable, actionTable, defaultValue */
    /*  XtEventsPtr  eventTable;    */
    /*  XtActionsPtr actionTable;   */
    /*  caddr_t      defaultValue;  */

extern XtEventsPtr XtParseEventBindings(); /* stringTable */
    /* char **stringTable */

extern caddr_t XtInterpretAction(); /* state, action */
    /* TranslationPtr start;    */
    /* XtAction       action;   */

XtActionTokenPtr XtTranslateEvent(); /* event, state */
    /* XEvent         *event;   */
    /* TranslationPtr state;    */



/***********************************************************************
 *
 * Utility routines
 *
 ***********************************************************************/

extern Window XtCreateWindow(); /* (dpy, parent, x, y, width, height, 
				    borderWidth, border, background,
				    bitgravity) */
    /* Display *dpy; */
    /* Window parent; */
    /* Position x, y; */
    /* Dimension width, height, borderWidth; */
    /* Pixel border; */
    /* Pixel background; */
    /* int bitgravity; */

#endif _Xtintrinsic_h
/* DON'T ADD STUFF AFTER THIS #endif */
