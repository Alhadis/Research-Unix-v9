/*
* $Header: Intrinsic.h,v 1.32 87/09/13 20:36:19 newman Exp $
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
 * Miscellaneous definitions
 *
 ****************************************************************/


#include	<X11/Xlib.h>
#include	<X11/Xresource.h>
#include	<sys/types.h>

#ifndef NULL
#define NULL 0
#endif


#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#define XtNumber(arr)		((Cardinal) (sizeof(arr) / sizeof(arr[0])))
#define XtOffset(type,field)    ((unsigned int)&(((type)NULL)->field))
typedef char *String;
typedef struct _WidgetRec *Widget;
typedef struct _WidgetClassRec *WidgetClass;
typedef struct _CompositeRec *CompositeWidget;
typedef struct _XtEventRec *_XtEventTable;
typedef struct _XtActionsRec *XtActionList;
typedef struct _XtResource *XtResourceList;
typedef struct _GrabRec  *GrabList;
typedef unsigned int   Cardinal;
typedef char	Boolean;
typedef unsigned long	*Opaque;
typedef struct _TranslationData	*_XtTranslations;
typedef struct _XtCallbackRec*    XtCallbackList;
typedef unsigned long   XtCallbackKind;
typedef unsigned long   XtValueMask;
typedef unsigned long   XtIntervalId;
typedef unsigned int    XtGeometryMask;
typedef unsigned long   XtGCMask;   /* Mask of values that are used by widget*/
typedef unsigned long   Pixel;	    /* Index into colormap	        */
typedef int		Position;   /* Offset from 0 coordinate	        */
typedef unsigned int	Dimension;  /* Size in pixels		        */
				    /* should be unsigned, but pcc      */
				    /* generates bad code for unsigned? */

typedef void (*XtProc)();
    /* takes no arguments */

typedef void (*XtWidgetProc)();
    /* Widget widget */

typedef void (*XtArgsProc)();
    /* Widget widget */
    /* ArgList args */
    /* Cardinal num_args */

typedef void (*XtInitProc)();
    /* Widget requestWidget; */
    /* Widget newWidget; */
    /* ArgList args */
    /* Cardinal num_args */

typedef Boolean (*XtSetValuesProc)();  /* returns TRUE if redisplay needed */
    /* Widget widget;     */
    /* Widget request;	  */
    /* Widget new;        */
    /* Boolean last;      */

typedef void (*XtExposeProc)();
    /* Widget    widget; */
    /* XEvent    *event; */

typedef void (*XtRealizeProc) ();
    /* Widget	widget;			    */
    /* XtValueMask mask;			    */
    /* XSetWindowAttributes *attributes;    */

typedef enum  {
    XtGeometryYes,        /* Request accepted. */
    XtGeometryNo,         /* Request denied. */
    XtGeometryAlmost,     /* Request denied, but willing to take replyBox. */
} XtGeometryResult;

typedef XtGeometryResult (*XtGeometryHandler)();
    /* Widget		    widget */
    /* XtWidgetGeometry       *request */
    /* XtWidgetGeometry	    *reply   */

/****************************************************************
 *
 * System Dependent Definitions
 *
 *
 * XtArgVal ought to be a union of caddr_t, char *, long, int *, and proc *
 * but casting to union types is not really supported.
 *
 * So the typedef for XtArgVal should be chosen such that
 *
 *      sizeof (XtArgVal) >=	sizeof(caddr_t)
 *				sizeof(char *)
 *				sizeof(long)
 *				sizeof(int *)
 *				sizeof(proc *)
 *
 * ArgLists rely heavily on the above typedef.
 *
 ****************************************************************/
typedef long XtArgVal;

/***************************************************************
 * Widget Core Data Structures
 *
 *
 **************************************************************/

typedef struct _CorePart {
    WidgetClass	    widget_class;	/* pointer to Widget's ClassRec	     */
    Widget	    parent;		/* parent widget	  	     */
    String          name;		/* widget resource name		     */
    XrmName         xrm_name;		/* widget resource name quarkified   */
    Screen	    *screen;		/* window's screen		     */
    Window	    window;		/* window ID			     */
    Position        x, y;		/* window position		     */
    Dimension       width, height;	/* window dimensions		     */
    Cardinal        depth;		/* number of planes in window        */
    Dimension       border_width;	/* window border width		     */
    Pixel	    border_pixel;	/* window border pixel		     */
    Pixmap          border_pixmap;	/* window border pixmap or NULL      */
    Pixel	    background_pixel;	/* window background pixel	     */
    Pixmap          background_pixmap;	/* window background pixmap or NULL  */
    _XtEventTable   event_table;	/* private to event dispatcher       */
    _XtTranslations translations;	/* private to Translation Manager    */
    Boolean         visible;		/* is window mapped and not occluded?*/
    Boolean	    sensitive;		/* is widget sensitive to user events*/
    Boolean         ancestor_sensitive;	/* are all ancestors sensitive?      */
    Boolean         managed;		/* is widget geometry managed?	     */
    Boolean	    mapped_when_managed;/* map window if it's managed?       */
    Boolean         being_destroyed;	/* marked for destroy		     */
    XtCallbackList  destroy_callbacks;	/* who to call when widget destroyed */
} CorePart;

typedef struct _WidgetRec {
    CorePart    core;
 } WidgetRec;

typedef Widget *WidgetList;

/******************************************************************
 *
 * Core Class Structure. Widgets, regardless of their class, will have
 * these fields.  All widgets of a given class will have the same values
 * for these fields.  Widgets of a given class may also have additional
 * common fields.  These additional fields are included in incremental
 * class structures, such as CommandClass.
 *
 * The fields that are specific to this subclass, as opposed to fields that
 * are part of the superclass, are called "subclass fields" below.  Many
 * procedures are responsible only for the subclass fields, and not for
 * any superclass fields.
 *
 ********************************************************************/

typedef struct _CoreClassPart {
    WidgetClass	    superclass;	       /* pointer to superclass ClassRec     */
    String          class_name;	       /* widget resource class name         */
    Cardinal        widget_size;       /* size in bytes of widget record     */
    XtProc	    class_initialize;  /* class initialization proc	     */
    Boolean         class_inited;      /* has class been initialized?        */
    XtInitProc      initialize;	       /* initialize subclass fields         */
    XtRealizeProc   realize;	       /* XCreateWindow for widget	     */
    XtActionList    actions;	       /* widget semantics name to proc map  */
    Cardinal	    num_actions;       /* number of entries in actions       */
    XtResourceList  resources;	       /* resources for subclass fields      */
    Cardinal        num_resources;     /* number of entries in resources     */
    XrmClass        xrm_class;	       /* resource class quarkified	     */
    Boolean         compress_motion;   /* compress MotionNotify for widget   */
    Boolean         compress_exposure; /* compress Expose events for widget  */
    Boolean         visible_interest;  /* select for VisibilityNotify        */
    XtWidgetProc    destroy;	       /* free data for subclass pointers    */
    XtWidgetProc    resize;	       /* geom manager changed widget size   */
    XtExposeProc    expose;	       /* rediplay window		     */
    XtSetValuesProc set_values;	       /* set subclass resource values       */
    XtWidgetProc    accept_focus;      /* assign input focus to widget       */
  } CoreClassPart;

typedef struct _WidgetClassRec {
    CoreClassPart core_class;
} WidgetClassRec;

extern WidgetClassRec widgetClassRec;
extern WidgetClass widgetClass;

/************************************************************************
 *
 * Additional instance fields for widgets of (sub)class 'Composite' 
 *
 ************************************************************************/

typedef Cardinal (*XtOrderProc)();
    /* Widget child; */


typedef struct _CompositePart {
    WidgetList  children;	     /* array of ALL widget children	     */
    Cardinal    num_children;	     /* total number of widget children	     */
    Cardinal    num_slots;           /* number of slots in children array    */
    Cardinal    num_mapped_children; /* count of managed and mapped children */
    XtOrderProc insert_position;     /* compute position of new child	     */
} CompositePart;

typedef struct _CompositeRec {
    CorePart      core;
    CompositePart composite;
} CompositeRec;

typedef struct _ConstraintPart {
    caddr_t     mumble;		/* No new fields, keep C compiler happy */
} ConstraintPart;

typedef struct _ConstraintRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart  constraint;
} ConstraintRec, *ConstraintWidget;

/*********************************************************************
 *
 *  Additional class fields for widgets of (sub)class 'Composite'
 *
 ********************************************************************/

typedef struct _CompositeClassPart {
    XtGeometryHandler geometry_manager;	  /* geometry manager for children   */
    XtWidgetProc      change_managed;	  /* change managed state of child   */
    XtArgsProc	      insert_child;	  /* physically add child to parent  */
    XtWidgetProc      delete_child;	  /* physically remove child	     */
    XtWidgetProc      move_focus_to_next; /* move Focus to next child	     */
    XtWidgetProc      move_focus_to_prev; /* move Focus to previous child    */
} CompositeClassPart;

typedef struct _CompositeClassRec {
     CoreClassPart      core_class;
     CompositeClassPart composite_class;
} CompositeClassRec, *CompositeWidgetClass;

extern CompositeClassRec compositeClassRec;
extern CompositeWidgetClass compositeWidgetClass;


typedef struct _ConstraintClassPart {
    XtResourceList constraints;	      /* constraint resource list	     */
    Cardinal   num_constraints;       /* number of constraints in list       */
} ConstraintClassPart;

typedef struct _ConstraintClassRec {
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
} ConstraintClassRec, *ConstraintWidgetClass;

extern ConstraintClassRec constraintClassRec;
extern ConstraintWidgetClass constraintWidgetClass;

/*************************************************************************
 *
 * Generic Procedures
 *
 *************************************************************************/


extern Boolean XtIsSubclass ();
    /* Widget       widget;	    */
    /* WidgetClass  widgetClass;    */

/* Some macros to get frequently used components of a widget */

#define XtDisplay(widget)	((widget)->core.screen->display)
#define XtScreen(widget)	((widget)->core.screen)
#define XtWindow(widget)	((widget)->core.window)
#define XtMapWidget(widget)	XMapWindow(XtDisplay(widget), XtWindow(widget))
#define XtUnmapWidget(widget)	\
		XUnmapWindow(XtDisplay(widget), XtWindow(widget))
#define XtIsComposite(widget)	\
		XtIsSubclass(widget, (WidgetClass)compositeWidgetClass)
#define XtClass(widget)		((widget)->core.widget_class)
#define XtSuperclass(widget)	(XtClass(widget)->core_class.superclass)

extern Widget XtCreateWidget ();
    /* String	    name;	    */
    /* WidgetClass  widgetClass;    */
    /* Widget       parent;	    */
    /* ArgList      args;	    */
    /* Cardinal     num_args;       */

extern Widget TopLevelCreate (); /*hack for now*/
    /* String	   name; */
    /* WidgetClass widgetClass; */
    /* Screen      *screen;*/
    /* ArgList     args; */
    /* Cardinal    num_args; */



extern void XtRealizeWidget ();
    /* Widget    widget      */

extern Boolean XtIsRealized ();
    /* Widget    widget; */

extern void XtDestroyWidget ();
    /* Widget widget */

extern void XtSetSensitive ();
    /* Widget    widget;    */
    /* Boolean   sensitive; */

extern void XtSetMappedWhenManaged ();
    /* Widget    widget;    */
    /* Boolean   mappedWhenManaged; */

/**********************************************************
 *
 * Composite widget Procedures
 *
 **********************************************************\


extern void XtManageChildren ();
    /* WidgetList children; */
    /* Cardinal   num_children; */

extern void XtManageChild ();
    /* Widget    child; */

extern void XtUnmanageChildren ();
    /* WidgetList children; */
    /* Cardinal   num_children; */

extern void XtUnmanageChild ();
    /* Widget child; */


/*************************************************************
 *
 *  Callbacks
 *
 **************************************************************/

typedef void (*XtCallbackProc)();
    /* Widget widget; */
    /* caddr_t closure;  data the application registered */
    /* caddr_t callData; widget instance specific data passed to application*/

typedef struct _XtCallbackRec {
    XtCallbackList next;
    Widget   widget;
    XtCallbackProc callback;
    Opaque  closure;
}XtCallbackRec;

extern XtCallbackKind XtNewCallbackKind();
    /* WidegtClass widgetClass; */
    /* Cardinal offset; */

extern void XtAddCallback ();
    /* Widget       widget;   */
    /* XtCallbackKind callbackKind; */
    /* XtCallbackProc callback; */
    /* caddr_t      closure;  */

extern void XtRemoveCallback ();
    /* Widget       widget;   */
    /* XtCallbackKind callbackKind; */
    /* XtCallbackProc callback; */
    /* caddr_t      closure;  */


extern void XtRemoveAllCallbacks ();
    /* Widget widget; */
    /* XtCallbackKind callbackKind; */

extern void XtCallCallbacks ();
    /* Widget  widget; */
    /* XtCallbackKind callbackKind; */
    /* caddr_t callData */

/****************************************************************
 *
 * Toolkit initialization
 *
 ****************************************************************/

extern Widget XtInitialize();
    /* XtAtom		    name;       */
    /* XtAtom		    class;      */
    /* XrmOptionsDescRec    options;    */
    /* Cardinal             num_options;  */
    /* Cardinal		    *argc; */ /* returns count of args not processed */
    /* char		    **argv;     */

/****************************************************************
 *
 * Memory Management
 *
 ****************************************************************/

extern char *XtMalloc(); /* size */
    /* Cardinal size; */

extern char *XtCalloc(); /* num, size */
    /* Cardinal num, size; */

extern char *XtRealloc(); /* ptr, num */
    /* char     *ptr; */
    /* Cardinal num; */

extern void XtFree(); /* ptr */
	/* char  *ptr */


/****************************************************************
 *
 * Arg lists
 *
 ****************************************************************/

typedef struct {
    String	name;
    XtArgVal	value;
} Arg, *ArgList;

#define XtSetArg(arg, n, d) \
    ( (arg).name = (n), (arg).value = (XtArgVal)(d) )

extern ArgList XtMergeArgLists(); /* args1, num_args1, args2, num_args2 */
    /* ArgList args1;       */
    /* int     num_args1;   */
    /* ArgList args2;       */
    /* int     num_args2;   */



/****************************************************************
 *
 * Event Management
 *
 ****************************************************************/

/* ||| Much of this should be private */
XtCallbackList DestroyList;
Display *toplevelDisplay;
typedef unsigned long EventMask;

typedef enum {pass,ignore,remap} GrabType;
typedef void (*XtEventHandler)(); /* widget, closure, event */
    /* Widget  widget   */
    /* caddr_t closure  */
    /* XEvent  *event;  */

typedef struct _XtEventRec {
     _XtEventTable next;
     EventMask   mask;
     Boolean     non_filter;
     XtEventHandler proc;
     Opaque closure;
}XtEventRec;

typedef struct _GrabRec {
    GrabList next;
    Widget  widget;
    Boolean  exclusive;
}GrabRec;

typedef struct _MaskRec {
    EventMask   mask;
    GrabType    grabType;
    Boolean     sensitive;
}MaskRec;
#define is_sensitive TRUE
#define not_sensitive FALSE
GrabRec *grabList;

extern EventMask _XtBuildEventMask(); /* widget */
    /* Widget widget; */

extern void XtAddEventHandler(); /* widget, eventMask, other, proc, closure */
    /* Widget		widget      */
    /* EventMask        eventMask;  */
    /* Boolean          other;      */
    /* XtEventHandler   proc;       */
    /* caddr_t		closure ;   */


extern void XtRemoveEventHandler(); /* widget,eventMask,other,proc,closure */
    /* Widget		widget      */
    /* EventMask        eventMask;  */
    /* Boolean          other;      */
    /* XtEventHandler   proc;       */
    /* caddr_t		closure ;   */


extern void XtDispatchEvent(); /* event */
    /* XEvent	*event; */

extern void XtMainLoop();

/****************************************************************
 *
 * Event Gathering Routines
 *
 ****************************************************************/

typedef unsigned long	XtInputMask;

#define XtInputNoneMask		0L
#define XtInputReadMask		(1L<<0)
#define XtInputWriteMask	(1L<<1)
#define XtInputExceptMask	(1L<<2)

extern Atom XtHasInput;
extern Atom XtTimerExpired;

extern XtIntervalId XtAddTimeOut();
    /* Widget widget;		*/
    /* unsigned long interval;  */

extern void XtRemoveTimeOut();
    /* XtIntervalId timer;      */

extern unsigned long XtGetTimeOut();
    /* XtIntervalId   timer;    */

extern void XtAddInput(); /* widget, source, condition */
    /* Widget widget		*/
    /* int source;		*/
    /* XtInputMask inputMask;	*/

extern void XtRemoveInput(); /* widget, source, condition */
    /* Widget widget		*/
    /* int source;		*/
    /* XtInputMask inputMask;	*/

extern void XtNextEvent(); /* event */
    /* XtEvent *event;		*/

extern XtPeekEvent(); /* event */
    /* XtEvent *event;		*/

extern Boolean XtPending ();


/****************************************************************
 *
 * Geometry Management
 *
 ****************************************************************/

#define XtDontChange	5 /* don't change the stacking order stack_mode */

typedef struct {
    XtGeometryMask request_mode;
    Position x, y;
    Dimension width, height, border_width;
    Widget sibling;
    int stack_mode;	/* Above, Below, TopIf, BottomIf, Opposite */
} XtWidgetGeometry;


extern XtGeometryResult XtMakeGeometryRequest();
    /*  widget, request, reply		    */
    /* Widget	widget; 		    */
    /* XtWidgetGeometry    *request;	    */
    /* XtWidgetGeometry	 *reply;  /* RETURN */

extern XtGeometryResult XtMakeResizeRequest ();
    /* Widget    widget;	*/
    /* Dimension width, height; */
    /* Dimension *replyWidth, *replyHeight; */

extern void XtResizeWindow(); /* widget */
    /* Widget widget; */

extern void XtResizeWidget(); /* widget, width, height, borderWidth */
    /* Widget  widget */
    /* Dimension width, height, borderWidth; */

extern void XtMoveWidget(); /* widget, x, y */
    /* Widget  widget */
    /* Position x, y  */


/****************************************************************
 *
 * Graphic Context Management
 *****************************************************************/

extern GC XtGetGC(); /* widget, valueMask, values */
    /* Widget    widget */
    /* XtGCMask valueMask; */
    /* XGCValues *values; */

extern void XtDestroyGC ();
    /* GC gc; */

/****************************************************************
 *
 * Resources
 *
 ****************************************************************/

#define StringToQuark(string) XrmAtomToQuark(string)
#define StringToName(string) XrmAtomToName(string)
#define StringToClass(string) XrmAtomToClass(string)

typedef struct _XtResource {
    String     resource_name;	/* Resource name			    */
    String     resource_class;	/* Resource class			    */
    String     resource_type;	/* Representation type desired		    */
    Cardinal    resource_size;	/* Size in bytes of representation	    */
    Cardinal    resource_offset;/* Offset from base to put resource value   */
    String     default_type;	/* representation type of specified default */
    caddr_t     default_addr;   /* Address of default resource		    */
} XtResource;


extern void XtGetResources();
    /* Widget       widget;             */
    /* ArgList	    args;		*/
    /* int	    num_args;		*/

extern void XtReadBinaryDatabase ();
    /* FILE    *f;			*/
    /* ResourceDatabase *db;		*/

extern void XtWriteBinaryDatabase ();
    /* FILE    *f;			*/
    /* ResourceDatabase db;		*/

extern void XtSetValues(); 
    /* Widget           widget;         */
    /* ArgList		args;		*/
    /* int		num_args;       */

extern void XtGetValues();
    /* Widget           widget;         */
    /* ArgList		args;		*/
    /* Cardinal 	num_args;       */

extern Widget XtStringToWidget ();
    /* String s; */

extern WidgetClass XtStringToClass ();
    /* String s; */



/****************************************************************
 *
 * Translation Management
 *
 ****************************************************************/

typedef struct _XtActionsRec{
    char    *string;
    caddr_t value;
} XtActionsRec;

/* ||| Should be private */
extern void XtDefineTranslation ();
    /* Widget widget */

/*************************************************************
 *
 * Error Handling
 *
 ************************************************************/


extern void XtSetErrorHandler(); /* errorProc */
  /* (*errorProc)(String); */

extern void XtError();  /* message */
    /* String message */

extern void XtSetWarningHandler(); /* errorProc */
  /* (*errorProc)(String); */

extern void XtWarning();  /* message */
    /* String message */


#endif _Xtintrinsic_h
/* DON'T ADD STUFF AFTER THIS #endif */
