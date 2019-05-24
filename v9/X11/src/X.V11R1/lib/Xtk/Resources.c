#ifndef lint
static char rcsid[] = "$Header: Resources.c,v 1.15 87/09/13 20:49:22 newman Exp $";
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
/* Converted to classing toolkit on 28 August 1987 by Joel McCormack */

/* XtResourceList.c -- compile and process resource lists. */


#include "Intrinsic.h"
#include "Atoms.h"
/*#include "Xresource.h"*/
#include <stdio.h>


#ifdef reverseVideoHack
static XrmName	QreverseVideo;
#endif
static XrmClass	QBoolean, QString;

extern void bcopy();

static void CopyFromArg(src, dst, size)
    XtArgVal src, dst;
    register unsigned int size;
{
    if (size == sizeof(XtArgVal))
	*(XtArgVal *)dst = src;
#ifdef BIGENDIAN
    else if (size == sizeof(short)) 
	*(short *)dst = (short)src;
#endif BIGENDIAN
    else if (size < sizeof(XtArgVal))
	bcopy((char *) &src, (char *) dst, (int) size);
    else
	bcopy((char *) src, (char *) dst, (int) size);

}

static void CopyToArg(src, dst, size)
    XtArgVal src, *dst;
    register unsigned int size;
{
    if (size == sizeof(XtArgVal))
	*dst = *(XtArgVal *)src;
#ifdef BIGENDIAN
    else if (size == sizeof(short)) 
	*dst = (XtArgVal) *((short *) src);
#endif BIGENDIAN
    else if (size < sizeof(XtArgVal))
	bcopy((char *) src, (char *) dst, (int) size);
    else
	bcopy((char *) src, (char *) *dst, (int) size);

}

static void PrintResourceList(list, count)
    register XtResourceList list;
    register int count;
{
    for (; --count >= 0; list++) {
        (void) printf("    name: %s, class: %s, type: %s,\n",
	    list->resource_name, list->resource_class, list->resource_type);
	(void) printf("    size: %d, offset: %x, def_type: %s, def_addr: %x\n",
	    list->resource_size, list->resource_offset,
	    list->default_type, list->default_addr);
    }
}

static Cardinal GetNamesAndClasses(w, names, classes)
    register Widget	  w;
    register XrmNameList  names;
    register XrmClassList classes;
{
    register Cardinal length, j;
    register XrmQuark t;

    for (length = 0; w != NULL; w = (Widget) w->core.parent) {
	names[length] = w->core.xrm_name;
	classes[length] = w->core.widget_class->core_class.xrm_class;
	length++;
     }
    /* They're in backwards order, flop them around */
    for (j = 0; j < length/2; j++) {
	t = names[j];
	names[j] = names[length-j-1];
	names[length-j-1] = t;
        t = classes[j];
	classes[j] = classes[length-j-1];
	classes[length-j-1] = t;
    }
    return length;
}


/* Spiffy fast compiled form of resource list.				*/
/* XtResourceLists are compiled in-place into XrmResourceLists		*/
/* All atoms are replaced by quarks, and offsets are -offset-1 to	*/
/* indicate that this list has been compiled already			*/

typedef struct {
    XrmQuark	xrm_name;	  /* Resource name quark 		*/
    XrmQuark	xrm_class;	  /* Resource class quark 		*/
    XrmQuark	xrm_type;	  /* Resource representation type quark */
    Cardinal	xrm_size;	  /* Size in bytes of representation	*/
    long int	xrm_offset;	  /* -offset-1				*/
    XrmQuark	xrm_default_type; /* Default representation type quark 	*/
    caddr_t	xrm_default_addr; /* Default resource address		*/
} XrmResource, *XrmResourceList;

static XrmCompileResourceList(resources, num_resources)
    register XtResourceList resources;
    	     Cardinal	  num_resources;
{
    register XrmResourceList xrmres;
    register Cardinal count;

    for (xrmres = (XrmResourceList) resources, count = 0;
         count < num_resources;
	 xrmres++, resources++, count++) {
    	xrmres->xrm_name	 = StringToName(resources->resource_name);
    	xrmres->xrm_class	 = StringToClass(resources->resource_class);
    	xrmres->xrm_type	 = StringToQuark(resources->resource_type);
	xrmres->xrm_size	 = resources->resource_size;
        xrmres->xrm_offset	 = -resources->resource_offset - 1;
    	xrmres->xrm_default_type = StringToQuark(resources->default_type);
	xrmres->xrm_default_addr = resources->default_addr;
    }

} /* XrmCompileResourceList */

/* ||| References to display should be references to screen */

static void XrmGetResources(
    dpy, base, names, classes, length, resources, num_resources, args, num_args)

    Display	  *dpy;		   /* The widget's display connection	  */
    caddr_t	  base;		   /* Base address of memory to write to  */
    register XrmNameList names;	   /* Full inheritance name of widget  	  */
    register XrmClassList classes; /* Full inheritance class of widget 	  */
    Cardinal	  length;	   /* Number of entries in names, classes */
    XtResourceList  resources;	   /* The list of resources required. 	  */
    Cardinal	  num_resources;   /* number of items in resource list    */
    ArgList 	  args;		   /* ArgList to override resources	  */
    Cardinal	  num_args;	   /* number of items in arg list	  */
{
    register 	ArgList		arg;
    register 	XrmName		argName;
		XrmResourceList	xrmres;
    register 	XrmResourceList	res;
    		XrmValue	val, defaultVal;
    register 	int		j;
    		int		i;
    		XrmHashTable	searchList[100];
    static	Boolean		found[1000];

#ifdef reverseVideoHack
		Boolean		reverseVideo, getReverseVideo;

    reverseVideo    = FALSE;
    getReverseVideo = TRUE;
#endif

    /* ||| This should be passed a compiled arg list, too, but such has to
       be allocated dynamically */

    /* ||| Should be warnings? or error? */
    if ((args == NULL) && (num_args != 0)) {
    	XtError("argument count > 0 on NULL argument list");
	num_args = 0;
    }
    if ((resources == NULL) && (num_resources != 0)) {
    	XtError("resource count > 0 on NULL resource list");
	return;
    }

    if (num_resources != 0) {
	/* Compile resource list if needed */
	if (((int)resources->resource_offset) >= 0) {
	    XrmCompileResourceList(resources, num_resources);
	}
	xrmres = (XrmResourceList) resources;

	/* Mark each resource as not found on arg list */
	for (j = 0; j < num_resources; j++) {
	    found[j] = FALSE;
    	}

	/* Copy the args into the resources, mark each as found */
	for (arg = args, i = 0; i < num_args; i++, arg++) {
	    argName = StringToName(arg->name);
#ifdef reverseVideoHack
	    if (argName == QreverseVideo) {
		reverseVideo = (Boolean) arg->value;
		getReverseVideo = FALSE;
	    } else
#endif
		for (j = 0, res = xrmres; j < num_resources; j++, res++) {
		    if (argName == res->xrm_name) {
			CopyFromArg(
			    arg->value,
			    (XtArgVal) base - res->xrm_offset - 1,
			    res->xrm_size);
			found[j] = TRUE;
			break;
		    }
		}
	}
    }

    /* Resources name and class will go into names[length], classes[length] */
    names[length+1]   = NULLQUARK;
    classes[length+1] = NULLQUARK;

#ifdef reverseVideoHack
    if (XDisplayCells(dpy, DefaultScreen(dpy)) > 2) {
    	/* Color box, ignore ReverseVideo */
	reverseVideo = FALSE;
    } else if (getReverseVideo) {
	names[length] = QreverseVideo;
	classes[length] = QBoolean;
	XrmGetResource(dpy, names, classes, QBoolean, &val);
	if (val.addr)
	    reverseVideo = *((Boolean *) val.addr);
    }
    /* ||| Nothing is done w/reverseVideo now, but something should be! */
#endif

    names[length] = NULLQUARK;
    classes[length] = NULLQUARK;

    if (num_resources != 0) {

	/* Ask resource manager for a list of database levels that we can
	   do a single-level search on each resource */

	XrmGetSearchList(names, classes, searchList);
	
	/* go to the resource manager for those resources not found yet */
	/* if it's not in the resource database use the default value   */
    
	for (res = xrmres, j = 0; j < num_resources; j++, res++) {
	    if (! found[j]) {
		XrmGetSearchResource(dpy, searchList, res->xrm_name,
		 res->xrm_class, res->xrm_type, &val);
/*
		if (val.addr == NULL && res->xrm_default_addr != NULL) {
*/
		if (val.addr == NULL) {
		    /* Convert default value to proper type */
		    defaultVal.addr = res->xrm_default_addr;
		    defaultVal.size = sizeof(caddr_t);
		    _XrmConvert(dpy, res->xrm_default_type, defaultVal, 
		    	res->xrm_type, &val);
		}
		if (val.addr) {
		    if (res->xrm_type == QString) {
			*((caddr_t *)(base - res->xrm_offset - 1)) = val.addr;
#ifdef BIGENDIAN
/* ||| Why? This should be handled by string to short, etc. conversions */
          	    } else if (res->xrm_size == sizeof(short)) {
		        *(short *) (base - res->xrm_offset - 1) =
				(short)*((int *)val.addr);
#endif BIGENDIAN
		    } else {
		        bcopy(
			    (char *) val.addr,
			    (char *) (base - res->xrm_offset - 1), 
			    (int) res->xrm_size);
		    }
		} else if (res->xrm_default_addr != NULL) {
		    bcopy(
			(char *) res->xrm_default_addr,
			(char *) (base - res->xrm_offset - 1),
			(int) res->xrm_size);
		} else {
		   /* didn't find a default value, initialize to NULL... */
		   bzero(
		       (char *) (base - res->xrm_offset - 1),
		       (int) res->xrm_size);
		}
	    }
	}
    }
}

static void GetResources(widgetClass, w, names, classes, length, args, num_args)
    WidgetClass	  widgetClass;
    Widget	  w;
    XrmNameList	  names;
    XrmClassList  classes;
    Cardinal	  length;
    ArgList	  args;
    Cardinal	  num_args;
{
    /* First get resources for superclasses */
    if (widgetClass->core_class.superclass != NULL) {
        GetResources(widgetClass->core_class.superclass,
	    w, names, classes, length, args, num_args);
    }
    /* Then for this class */
    XrmGetResources(XtDisplay(w), (caddr_t) w, names, classes, length,
        widgetClass->core_class.resources, widgetClass->core_class.num_resources,
	args, num_args);
} /* GetResources */


void XtGetResources(w, args, num_args)
    register 	Widget	  w;
    		ArgList	  args;
    		Cardinal  num_args;
{
    XrmName	names[100];
    XrmClass	classes[100];
    Cardinal	length;

    /* Make sure xrm_class, xrm_name are valid */
    if (w->core.widget_class->core_class.xrm_class == NULLQUARK) {
        w->core.widget_class->core_class.xrm_class =
	    StringToClass(w->core.widget_class->core_class.class_name);
    }
    w->core.xrm_name = StringToName(w->core.name);

    /* Get names, classes for widget on up */
    length = GetNamesAndClasses(w, names, classes);
   
    /* Get resources starting at CorePart on down to this widget */
    GetResources(w->core.widget_class, w, names, classes, length,
        args, num_args);
} /* XtGetResources */

void XtGetSubresources
	(w, base, name, class, resources, num_resources, args, num_args)
    Widget	  w;		  /* Widget "parent" of subobject */
    caddr_t	  base;		  /* Base address to write to     */
    String	  name;		  /* name of subobject		  */
    String	  class;	  /* class of subobject		  */
    XtResourceList  resources;	  /* resource list for subobject  */
    Cardinal	  num_resources;
    ArgList	  args;		  /* arg list to override resources */
    Cardinal	  num_args;
{
    XrmName	  names[100];
    XrmClass	  classes[100];
    Cardinal	  length;

    /* Get full name, class of subobject */
    length = GetNamesAndClasses(w, names, classes);
    names[length] = StringToName(name);
    classes[length] = StringToClass(class);
    length++;

    /* Fetch resources */
    XrmGetResources(XtDisplay(w), base, names, classes, length,
        resources, num_resources, args, num_args);
}


static void XrmGetValues(base, resources, num_resources, args, num_args)
  caddr_t		base;		/* Base address to fetch values from */
  register XtResourceList resources;	/* The current resource values.      */
  register Cardinal	num_resources;	/* number of items in resources      */
  ArgList 		args;		/* The resource values requested     */
  Cardinal		num_args;	/* number of items in arg list       */
{
    register ArgList		arg;
    register XrmResourceList	xrmres;
    register int 		i;
    register XrmName		argName;

    if (num_resources == 0) return;

    /* Resource lists are assumed to be in compiled form already via the
       initial XtGetResources, XtGetSubresources calls */

    for (arg = args ; num_args != 0; num_args--, arg++) {
	argName = StringToName(arg->name);
	for (xrmres = (XrmResourceList) resources, i = 0;
	     i < num_resources;
	     i++, xrmres++) {
	    if (argName == xrmres->xrm_name) {
		CopyToArg(
		    (XtArgVal) base - xrmres->xrm_offset - 1,
		    &arg->value,
		    xrmres->xrm_size);
		break;
	    }
	}
    }
}

static void GetValues(widgetClass, w, args, num_args)
    WidgetClass	  widgetClass;
    Widget	  w;
    ArgList	  args;
    Cardinal	  num_args;
{
    /* First get resource values for superclass */
    if (widgetClass->core_class.superclass != NULL) {
        GetValues(widgetClass->core_class.superclass, w, args, num_args);
    }
    /* Then for this class */
    XrmGetValues(
	(caddr_t) w,
        widgetClass->core_class.resources,
	widgetClass->core_class.num_resources,
	args, num_args);
} /* GetValues */

void XtGetValues(w, args, num_args)
    	 	Widget	  w;
    		ArgList	  args;
    		Cardinal  num_args;
{
    if (num_args == 0) return;
    if ((args == NULL) && (num_args != 0)) {
	XtError("argument count > 0 on NULL argument list");
	return;
    }
    /* Get resource values starting at CorePart on down to this widget */
    GetValues(w->core.widget_class, w, args, num_args);
} /* XtGetValues */
 
static void XrmSetValues(base, resources, num_resources, args, num_args)
  caddr_t		base;		/* Base address to write values to   */
  register XtResourceList resources;	/* The current resource values.      */
  register Cardinal	num_resources;	/* number of items in resources      */
  ArgList 		args;		/* The resource values to set        */
  Cardinal		num_args;	/* number of items in arg list       */
{
    register ArgList		arg;
    register XrmResourceList	xrmres;
    register int 	        i;
    register XrmName		argName;

    if (num_resources == 0) return;

    /* Resource lists are assumed to be in compiled form already via the
       initial XtGetResources, XtGetSubresources calls */

    for (arg = args ; num_args != 0; num_args--, arg++) {
	argName = StringToName(arg->name);
	for (xrmres = (XrmResourceList) resources, i = 0;
	     i < num_resources;
	     i++, xrmres++) {
	    if (argName == xrmres->xrm_name) {
		CopyFromArg(arg->value,
			    (XtArgVal) base - xrmres->xrm_offset - 1,
			    xrmres->xrm_size);
		break;
	    }
	}
    }
} /* XrmSetValues */

static void SetValues(widgetClass, w, args, num_args)
    WidgetClass	  widgetClass;
    Widget	  w;
    ArgList	  args;
    Cardinal	  num_args;
{
    /* First set resource values for superclass */
    if (widgetClass->core_class.superclass != NULL) {
        SetValues(widgetClass->core_class.superclass, w, args, num_args);
    }
    /* Then for this class */
    XrmSetValues((caddr_t) w,
        widgetClass->core_class.resources,
	widgetClass->core_class.num_resources,
	args, num_args);
} /* SetValues */

static Boolean RecurseSetValues (current, request, new, last, class)
    Widget current, request, new;
    Boolean last;
    WidgetClass class;
{
    Boolean redisplay = FALSE;
    if (class->core_class.superclass)
       	redisplay = RecurseSetValues (current, request, new, FALSE,
           class->core_class.superclass);
    if (class->core_class.set_values)
       	redisplay |= (*class->core_class.set_values) (current, request, new,
           last, class);
    return (redisplay);
}

void XtSetValues(w, args, num_args)
    Widget   w;
    ArgList  args;
    Cardinal num_args;
{
    Widget	requestWidget, newWidget;
    Cardinal	widgetSize;
    Boolean redisplay;

    if (num_args == 0) return;
    if ((args == NULL) && (num_args != 0)) {
	XtError("argument count > 0 on NULL argument list");
	return;
    }

    /* Allocate and copy current widget into newWidget */
    widgetSize = w->core.widget_class->core_class.widget_size;
    requestWidget = (Widget) XtMalloc (widgetSize);
    newWidget = (Widget) XtMalloc(widgetSize);
    bcopy((char *) w, (char *) requestWidget, widgetSize);

    /* Set resource values starting at CorePart on down to this widget */
    SetValues(w->core.widget_class, requestWidget, args, num_args);
    bcopy ((char *) requestWidget, (char *) newWidget, widgetSize);

    /* Inform widget of changes and deallocate newWidget */
    redisplay = RecurseSetValues (w, requestWidget, newWidget, TRUE,
        w->core.widget_class);
    
    /* If a SetValues proc made a successful geometry request,
       then "w" contains the new, correct x, y, width, height fields.
       Make sure not to smash them when copying "new" back into "w". */
    newWidget->core.x = w->core.x;
    newWidget->core.y = w->core.y;
    newWidget->core.width = w->core.width;
    newWidget->core.height = w->core.height;
    newWidget->core.border_width = w->core.border_width;

    bcopy ((char *) newWidget, (char *) w, widgetSize);
    XtFree((char *) requestWidget);
    XtFree((char *) newWidget);

    if (redisplay)
       /* repaint background of window, and force a full exposure event */
       XClearArea (XtDisplay(w), XtWindow(w), 0, 0, 0, 0, TRUE);
       
} /* XtSetValues */
 

static Boolean initialized = FALSE;

extern void ResourceListInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

#ifdef reverseVideoHack
    QreverseVideo = StringToName(XtNreverseVideo);
#endif
    QBoolean = StringToClass(XtCBoolean);
    QString = StringToClass(XtCString);
}
