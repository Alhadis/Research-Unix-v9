#ifndef lint
static char rcsid[] = "$Header: Initialize.c,v 1.44 87/09/13 22:55:11 newman Exp $";
#endif lint

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
/* Make sure all wm properties can make it out of the resource manager */

#include <stdio.h>
#include <pwd.h>
#include <sys/param.h>

 /* Xlib definitions  */
 /* things like Window, Display, XEvent are defined herein */
#include "Intrinsic.h"
#include <X11/Xutil.h>
#include "Atoms.h"
#include "TopLevel.h"

extern char *index();
extern char *strcat();
extern char *strncpy();
extern char *strcpy();

/*
 This is a set of default records describing the command line arguments that
 Xlib will parse and set into the resource data base.
 
 This list is applied before the users list to enforce these defaults.  This is
 policy, which the toolkit avoids but I hate differing programs at this level.
*/

static XrmOptionDescRec opTable[] = {
{"=",		XtNgeometry,	XrmoptionIsArg,		(caddr_t) NULL},
{"-bd",		XtNborder,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bordercolor",XtNborder,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bg",		XtNbackground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-background",	XtNbackground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bw",		XtNborderWidth,	XrmoptionSepArg,	(caddr_t) NULL},
{"-border",	XtNborderWidth,	XrmoptionSepArg,	(caddr_t) NULL},
{"-fg",		XtNforeground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-foreground",	XtNforeground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-fn",		XtNfont,	XrmoptionSepArg,	(caddr_t) NULL},
{"-font",	XtNfont,	XrmoptionSepArg,	(caddr_t) NULL},
{"-rv",		XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "on"},
{"-reverse",	XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "on"},
{"+rv",		XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "off"},
{"-n",		XtNname,	XrmoptionSepArg,	(caddr_t) NULL},
{"-name",	XtNname,	XrmoptionSepArg,	(caddr_t) NULL},
{"-title",	XtNtitle,	XrmoptionSepArg,	(caddr_t) NULL},
{"-t",		XtNtitle,	XrmoptionSepArg,	(caddr_t) NULL}
};

typedef struct {
	int	    argc;
	char      **argv;
	char	   *classname;
	char	   *icon_name;
	char	   *title;
	Pixmap	    icon_pixmap;
	Window	    icon_window;
	Boolean	    iconic;
	Boolean	    input;
	Boolean	    resizeable;
	char       *geostr;
	int	    initial;
	XSizeHints  hints;
} TopLevelPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef  struct {
	CorePart 	core;
	CompositePart 	composite;
	TopLevelPart 	top;
} TopLevelRec, *TopLevelWidget;

static XtResource resources[]=
{
	{ XtNiconName, XtCIconPixmap, XrmRString, sizeof(caddr_t),
	    XtOffset(TopLevelWidget, top.icon_name), XrmRString, (caddr_t) NULL},
	{ XtNiconPixmap, XtCIconPixmap, XrmRPixmap, sizeof(caddr_t),
	    XtOffset(TopLevelWidget, top.icon_pixmap), XrmRPixmap, 
	    (caddr_t) NULL},
	{ XtNiconWindow, XtCIconWindow, XrmRWindow, sizeof(caddr_t),
	    XtOffset(TopLevelWidget, top.icon_window), XrmRWindow, 
	    (caddr_t) NULL},
	{ XtNallowtopresize, XtCAllowtopresize, XrmRBoolean, sizeof(Boolean),
	    XtOffset(TopLevelWidget, top.resizeable), XrmRString, "FALSE"},
	{ XtNgeometry, XtCGeometry, XrmRString, sizeof(caddr_t), 
	    XtOffset(TopLevelWidget, top.geostr), XrmRString, (caddr_t) NULL},
	{ XtNinput, XtCInput, XrmRBoolean, sizeof(Boolean),
	    XtOffset(TopLevelWidget, top.input), XrmRString, "FALSE"},
	{ XtNiconic, XtCIconic, XrmRBoolean, sizeof(Boolean),
	    XtOffset(TopLevelWidget, top.iconic), XrmRBoolean, "FALSE"},
	{ XtNtitle, XtCTitle, XrmRString, sizeof(char *),
	    XtOffset(TopLevelWidget, top.title), XrmRString, NULL},
/*	{ XtNinitial, XtCInitial, XrmRInitialstate, sizeof(int),
	    XtOffset(TopLevelWidget, top.initial), XrmRString, "Normal"} */
	{ XtNinitial, XtCInitial, XrmRInt, sizeof(int),
	    XtOffset(TopLevelWidget, top.initial), XrmRString, "0"}
			/* ||| Temp hack to provide initialization. */
};
static void Initialize();
static void Realize();
static Boolean SetValues();
static void Destroy();
static void InsertChild();
static void ChangeManaged(); /* XXX */
static XtGeometryResult GeometryManager();
static void EventHandler();
static void ClassInitialize();

typedef struct _TopLevelClassRec {
  	CoreClassPart      core_class;
	CompositeClassPart composite_class;
} TopLevelClassRec;


TopLevelClassRec topLevelClassRec = {
    /* superclass         */    (WidgetClass) &compositeClassRec,
    /* class_name         */    "No Name",
				/* toplevel doesn't have a name it is pass
       				 * to XtInitialize
				 */
    /* size               */    sizeof(TopLevelRec),
    /* Class Initializer  */	ClassInitialize,
    /* Class init'ed ?    */	FALSE,
    /* initialize         */    Initialize,
    /* realize            */    Realize,
    /* actions            */    NULL,
    /* num_actions        */    0,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE,
    /* compress_exposure  */    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    NULL,
    /* expose             */    NULL,
    /* set_values         */    SetValues,
    /* accept_focus       */    NULL,
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	InsertChild,
    /* delete_child	  */	NULL,	      /* ||| Inherit from Composite */
    /* move_focus_to_next */    NULL,
    /* move_focus_to_prev */    NULL
};

WidgetClass topLevelWidgetClass = (WidgetClass) (&topLevelClassRec);

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

/* this is the old initialize routine */
/* This needs to be updated to reflect the new world. */
static void
DO_Initialize() {

extern void QuarkInitialize();
extern void ResourceListInitialize();
extern void EventInitialize();
extern void TranslateInitialize();
extern void CursorsInitialize();

    /* Resource management initialization */
    QuarkInitialize();
    XrmInitialize();
    ResourceListInitialize();

    /* Other intrinsic intialization */
    EventInitialize();
    TranslateInitialize();
    CursorsInitialize();
    InitializeCallbackTable();


}
Atom XtHasInput;
Atom XtTimerExpired;

static void
init_atoms(dpy)
Display *dpy;
{
	XtHasInput = XInternAtom(dpy, "XtHasInput", False);
	XtTimerExpired = XInternAtom(dpy, "XtTimerExpired", False);
}


static void ClassInitialize()
{
    CompositeWidgetClass superclass;
    TopLevelWidgetClass myclass;

    myclass = (TopLevelWidgetClass) topLevelWidgetClass;
    superclass = (CompositeWidgetClass) myclass->core_class.superclass;

    /* Inherit  delete_child from Composite */
    /* I have a insert_child that calls my parents superclasses insert_child */
    /* I can't inherit this one because of the implied add that Joel wants */

    myclass->composite_class.delete_child =
        superclass->composite_class.delete_child;
}

/*
    What this routine does load the resource data base.

    The order that things are loaded into the database follows,
    1) an optional application specific file.
    2) server defaults, generic.
    3) server defaults for machine.
    4) .Xdefaults

 */

static XrmResourceDataBase
XGetUsersDataBase(dpy, name)
Display *dpy;
char *name;
{
	XrmResourceDataBase userResources = NULL;
	XrmResourceDataBase resources = NULL;
	int uid;
	extern struct passwd *getpwuid();
	struct passwd *pw;
	char filename[1024];
	FILE *f;
	static Boolean first = TRUE;
	
	if(name != NULL) { /* application provided a file */
		f = fopen(name, "r");
		if (f) {
			XrmGetCurrentDataBase(&resources);
			XrmGetDataBase(f, &userResources);
			XrmMergeDataBases(userResources, &resources);
			XrmSetCurrentDataBase(userResources);
			(void) fclose(f);
		}
	} 
	if (first) {
		first = FALSE;
		/*
		  MERGE server defaults
		 */
	/* Open .Xdefaults file and merge into existing data base */
	
		uid = getuid();
		pw = getpwuid(uid);
		if (pw) {
			(void) strcpy(filename, pw->pw_dir);
			(void) strcat(filename, "/.Xdefaults");
			f = fopen(filename, "r");
			if (f) {
				XrmGetCurrentDataBase(&resources);
				XrmGetDataBase(f, &userResources);
				XrmMergeDataBases(userResources, &resources);
				XrmSetCurrentDataBase(userResources);
				(void) fclose(f);
			}
		}
	}
    return userResources;
}

static void
Initialize(request, new) 
Widget request, new;
{
	TopLevelWidget w = (TopLevelWidget) new;
	
	int flag;

	if(w->top.icon_name == NULL) {
		w->top.icon_name = w->core.name;
	}
	if(w->top.title == NULL) {
		w->top.title = w->top.icon_name;
	}
	w->top.hints.flags = 0;
	w->core.background_pixmap = None;
/*
	w->core.border_width = 0;
	w->core.border_pixmap = NULL;
*/
	if(w->top.geostr != NULL) {
		flag = XParseGeometry(w->top.geostr, &w->core.x, &w->core.y,
			       &w->core.width, &w->core.height);
		
		w->top.hints.flags |= USPosition | USSize;
		if(flag & XNegative) 
			w->core.x =
			  w->core.screen->width - w->core.width - w->core.x;
		if(flag & YNegative) 
			w->core.y = 
			 w->core.screen->height - w->core.height - w->core.y;
	}

	XtAddEventHandler(
	    new, (EventMask) StructureNotifyMask,
	    FALSE, EventHandler, (Opaque) NULL);
}

static void
Realize(wid, mask, attr)
Widget wid;
Mask mask;
XSetWindowAttributes *attr;
{
	TopLevelWidget w = (TopLevelWidget) wid;
	Window win;
	Display *dpy = XtDisplay(w);
/*
	char hostname[1024];
*/
	XWMHints wmhints;
	

	mask &= ~(CWBackPixel);
	mask |= CWBackPixmap;
	attr->background_pixmap = None;	/* I must have a background pixmap of
					   none, and no background pixel. 
					   This should give me a transparent
					   background so that if there is
					   latency from when I get resized and
					   when my child is resized it won't be
					   as obvious.
					   */
	win = w->core.window = XCreateWindow( dpy,
	    w->core.screen->root, w->core.x, w->core.y,
	    w->core.width, w->core.height,
	    w->core.border_width, w->core.depth, CopyFromParent,
	    CopyFromParent, mask, attr);

	XStoreName(dpy, win, w->top.title);
	XSetIconName(dpy, win, w->top.icon_name);
	XSetCommand(dpy, win, w->top.argv, w->top.argc);
#ifdef UNIX
	gethostname(hostname, sizeof(hostname));
#endif

/* now hide everything needed to set the properties until realize is called */
	wmhints.flags = 0;
	wmhints.flags |= InputHint ;
	if(w->top.input)
	  wmhints.input = TRUE;
	else
	  wmhints.input = FALSE;

	/* Should I tell the window manager to bring me up iconfied */
	wmhints.initial_state = w->top.initial;
	if(w->top.iconic)
	  wmhints.initial_state = IconicState;
	if(w->top.icon_pixmap != NULL) {
		wmhints.flags |=  IconPixmapHint;
		wmhints.icon_pixmap = w->top.icon_pixmap;
	}
	if(w->top.icon_window != NULL) {
		wmhints.flags |=  IconWindowHint;
		wmhints.icon_window = w->top.icon_window;
	}
	XSetWMHints(dpy, win, &wmhints);
/* |||
	XSetHostName(dpy, win, hostname);
	XSetClass(dpy, win, w->top.classname);/* And w->core.name XXX*/

	w->top.hints.x = w->core.x;
	w->top.hints.y = w->core.y;
	w->top.hints.width = w->core.width;
	w->top.hints.height = w->core.height;
	XSetNormalHints(dpy, win, &w->top.hints);
}

/* ARGSUSED */
static void
EventHandler(wid, closure, event)
Widget wid;
Opaque closure;
XEvent *event;
{
	Widget childwid;
	int i;
	TopLevelWidget w = (TopLevelWidget) wid;

	if(w->core.window != event->xany.window) {
		XtError("Event with wrong window");
		  return;
	}
	/* I am only interested in resize */
	switch(event->type) {
	      case ConfigureNotify:
		w->core.width = event->xconfigure.width;
		w->core.height = event->xconfigure.height;
		w->core.border_width = event->xconfigure.border_width;
		w->core.x = event->xconfigure.x;
		w->core.y = event->xconfigure.y;

		for(i = 0; i < w->composite.num_children; i++) {
		    if(w->composite.children[i]->core.managed) {
			  childwid = w->composite.children[i];
			  XtResizeWidget(
			      childwid,
			      w->core.width,
			      w->core.height,
			      w->core.border_width);
			  break;
		    }
		}
		break;
	      default:
		return;
	}  
}

static void
Destroy(wid)
Widget wid;
{
	TopLevelWidget w = (TopLevelWidget) wid;

	if(w->top.argv != NULL)
		XtFree((char *)w->top.argv);
	w->top.argv = NULL;
	if(w->top.classname != NULL)
		XtFree(w->top.classname);
}

/* ARGSUSED */
static void InsertChild(w, args, num_args)
    Widget w;
    ArgList args;
    Cardinal num_args;
{
    ((CompositeWidgetClass) XtSuperclass(w->core.parent))
	->composite_class.insert_child(w, args, num_args);
    XtManageChild(w);	/* Add to managed set now */
}

/*
 * There is some real ugliness here.  If I have a width and a height which are
 * zero, and as such suspect, and I have not yet been realized then I will 
 * grow to match my child.
 *
 */
static void 
ChangeManaged(wid)
CompositeWidget wid;
{
    TopLevelWidget w = (TopLevelWidget) wid;
    int     i;
    Widget childwid;

    for (i = 0; i < w->composite.num_children; i++) {
	if (w->composite.children[i]->core.managed) {
	    childwid = w->composite.children[i];
	    if (!XtIsRealized ((Widget) wid)) {
		if (w->core.border_width == 0) {
		    w->core.border_width = childwid->core.border_width;
		} else {
		    childwid->core.border_width = w->core.border_width;
		}
		if (w->core.width == 0
			&& w->core.height == 0) {
	            /* we inherit our child's attributes */
		    w->core.width = childwid->core.width;
		    w->core.height = childwid->core.height;
		    w->top.hints.flags |= PSize;
		} else {
		    /* our child gets our attributes */
		    XtResizeWidget (
		        childwid,
			w->core.width,
			w->core.height,
			w->core.border_width);
		}
	    }
	    if (childwid->core.x != -(w->core.border_width) ||
		childwid->core.y != -(w->core.border_width)) {
		XtMoveWidget (childwid,
			      -(w->core.border_width),
			      -(w->core.border_width));
	    }
	}
    }

}

/*
 * This is gross, I can't wait to see if the change happened so I will ask
 * the window manager to change my size and do the appropriate X work.
 * I will then tell the requester that he can't.  Care must be taken because
 * it is possible that some time in the future the request will be
 * asynchronusly granted.
 */
 
static XtGeometryResult
GeometryManager( wid, request, reply )
Widget wid;
XtWidgetGeometry *request;
XtWidgetGeometry *reply;
{
  	XWindowChanges values;
	XSizeHints	oldhints;
	TopLevelWidget w = (TopLevelWidget)(wid->core.parent);

	if(w->top.resizeable == FALSE)
		return(XtGeometryNo);
	if(!XtIsRealized((Widget)w)){
		if (request->request_mode & (CWX|CWY)) {
			if(request->request_mode & (CWX|CWY) == 
			   request->request_mode) {
				return(XtGeometryNo);
			} else {
				*reply = *request;
				reply->request_mode = request->request_mode &
				  ~(CWX|CWY);
				return(XtGeometryAlmost);
			}
		}
		*reply = *request;
		if(request->request_mode & CWWidth)
		   w->core.width = request->width;
		if(request->request_mode & CWHeight) 
		   w->core.height = request->height;
		if(request->request_mode & CWBorderWidth)
		   w->core.border_width = request->border_width;
		return(XtGeometryYes);
	}
	values = *(XWindowChanges *) (&(request->x));
	XGrabServer(XtDisplay(w));
	XGetNormalHints(XtDisplay(w), w->core.window, &oldhints);
        if(request->request_mode & CWWidth) {
                oldhints.flags &= ~USSize;
                oldhints.flags |= PSize;
                oldhints.width = request->width;
        }
        if(request->request_mode & CWHeight) {
                oldhints.flags &= ~USSize;
                oldhints.flags |= PSize;
                oldhints.height = request->height;
        }
        XSetNormalHints(XtDisplay(w), w->core.window, &oldhints);
/* ||| this code depends on the core x,y,width,height,borderwidth fields */
/* being the same size and same order as an XWindowChanges record. Yechh!!! */
	XConfigureWindow(XtDisplay(w), w->core.window,
		 request->request_mode, (XWindowChanges *)&(request->x));
	XUngrabServer(XtDisplay(w));
	return(XtGeometryNo);
}

static Boolean SetValues (current, request, new, last)
Widget current, request, new;
Boolean last;
{
	XWMHints wmhints, *oldhints;
	TopLevelWidget cur = (TopLevelWidget) current;
        TopLevelWidget req = (TopLevelWidget) request;
	TopLevelWidget tnew = (TopLevelWidget) new;
	Boolean name = FALSE;
	Boolean pixmap = FALSE;
	Boolean window = FALSE;
	Boolean title = FALSE;
	
        /* I don't let people play with most of my values */
        tnew->top = cur->top;

        /* except these few.... */
        
	tnew-> top.resizeable =  req->top.resizeable;
	if(req ->top.icon_name != cur->top.icon_name) {
		tnew ->top.icon_name = req->top.icon_name;
		name = TRUE;
	}
/*XXX Leak allert  These should be copied and freed but I am lazy */
	if(req ->top.icon_pixmap != cur->top.icon_pixmap) {
		tnew ->top.icon_pixmap = req->top.icon_pixmap;
		pixmap = TRUE;
	}
	if(req ->top.icon_window != cur->top.icon_window) {
		tnew ->top.icon_window = req->top.icon_window;
		window = TRUE;
	}
	if(req ->top.title != cur->top.title) {
		tnew ->top.title = req->top.title;
		name = TRUE;
	}
	if((name || pixmap || window || title) && XtIsRealized((Widget)tnew)) {
		if(name) {
			XSetIconName(XtDisplay(tnew), tnew->core.window, tnew->top.icon_name);
		}
		if( title ) {
			XStoreName(XtDisplay(tnew), tnew->core.window, tnew->top.title);
		}
		if(pixmap || window) {
			oldhints = XGetWMHints(XtDisplay(tnew), tnew->core.window);
			wmhints = *oldhints;
			XtFree((char *)oldhints);
			if(pixmap) {
				wmhints.flags |= IconPixmapHint;
				wmhints.icon_pixmap = tnew->top.icon_pixmap;
			}
			if(window) {
				wmhints.flags |= IconWindowHint;
				wmhints.icon_window = tnew->top.icon_window;
			}
			XSetWMHints( XtDisplay(tnew), tnew->core.window, &wmhints);
		}
	}

return (FALSE);  /* redisplay is never needed */
	
}

/*
 * This routine creates the desired widget and does the "Right Thing" for
 * the toolkit and for window managers.
 */

Widget
XtInitialize(name, classname, urlist, urlistCount, argc, argv)
char *name;
char *classname;
XrmOptionDescRec *urlist;
Cardinal urlistCount;
Cardinal *argc;
char *argv[];
{
	char  displayName[256];
/*
	char *displayName_ptr = displayName;
*/
	Arg   args[8];
	Cardinal num_args = 0;
	int i;
/*
	int val;
	int flags = 0;
*/
	char filename[MAXPATHLEN];
	char **saved_argv;
	int    saved_argc = *argc;
	Display *dpy;
	char *ptr, *rindex();
	TopLevelWidget w;
	Widget root;
	int squish = -1;
	Boolean dosync = FALSE;


	if( name == NULL) {
	  	ptr = rindex(argv[0], '/');
		if(ptr)
		  name = ++ ptr;
		else
		  name = argv[0];
	}

	/* save away argv and argc so I can set the properties latter */

	saved_argv = (char **) XtCalloc(
	    (unsigned) ((*argc) + 1) , (unsigned)sizeof(*saved_argv));
	for (i = 0 ; i < *argc ; i++)
	  saved_argv[i] = argv[i];
	saved_argv[i] = NULL;
	/*
	   Find the display name and open it
	   While we are at it we look for name because that is needed 
	   soon after to do the arguement parsing.
	 */
	displayName[0] = 0;

	for(i = 1; i < *argc; i++) {
	  if (index(argv[i], ':') != NULL) {
		  (void) strncpy(displayName, argv[i], sizeof(displayName));
		  if( *argc == i + 1) {
		    (*argc)--;
		  } else {  /* need to squish this one out of the list */
		    squish = i;
		  }
		  continue;
	  }
	  if(!strcmp("-name", argv[i]) || ! strcmp("-n", argv[i])) {
		  i++;
		  if(i == *argc) break;
		  name = argv[i];
		  continue;
	  }
	  if (!strcmp("-sync", argv[i])) {
		  dosync = TRUE;
		  continue;
	  }
	}
	if(squish != -1) {
		(*argc)--;
		for(i = squish; i < *argc; i++) {
			argv[i] = argv[i+1];
		}
	}
	/* Open display  */
	if (!(dpy = XOpenDisplay(displayName))) {
		char buf[1024];
		(void) strcpy(buf, "Can't Open display: ");
		(void) strcat(buf, displayName);
		XtError(buf);
	}
        toplevelDisplay = dpy;
	if (dosync) XSynchronize(dpy, TRUE);

        XtSetArg(args[num_args], "display", dpy);
        num_args++;
        XtSetArg(args[num_args], "screen", dpy->default_screen);
        num_args++;
	    
	/* initialize the toolkit */
	DO_Initialize();
#define UNIX
#ifdef UNIX
#define XAPPLOADDIR  "/usr/lib/Xapps/"
#endif	
	(void) strcpy(filename, XAPPLOADDIR);
	(void) strcat(filename, classname);


	/*set up resource database */
	XGetUsersDataBase(dpy, filename);

	/*
	   This routine parses the command line arguments and removes them from
	   argv.
	 */
	XrmParseCommand( opTable, XtNumber(opTable), name, argc, argv);
	
	if(urlistCount >0) {
		/* the application has some more defaults */
		XrmParseCommand( urlist, urlistCount, name, argc, argv);
	}
	/* Resources are initialize and loaded */
	/* I now must handle geometry specs a compond resource */

	/*
	     Create the top level widget.
	     Unlike most classes the toplevel widget class has no classname
	     The name is supplied in the call to XtInitialize.
	 */
	(void) strcpy(
	    ((TopLevelClassRec *)(topLevelWidgetClass))->core_class.class_name
	        = (String)XtMalloc((unsigned)strlen(classname)+1),
	       classname);
	root = TopLevelCreate(name, topLevelWidgetClass,
			      &(dpy->screens[dpy->default_screen]),
			       args, num_args);

	w = (TopLevelWidget) root;
	w->top.argc = saved_argc;
	w->top.argv = saved_argv;
	(void) strcpy(w->top.classname = (char *)XtMalloc((unsigned)strlen(classname)+1)
	       ,classname);

	init_atoms(dpy);

	return(root);
}

