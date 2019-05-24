#include "copyright.h"

/* $Header: XErrDes.c,v 11.15 87/06/14 18:00:13 jg Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include <stdio.h>
#include "Xlibint.h"
#include "Xresource.h"
#include "Quarks.h"
#include <strings.h>

char *XErrorList[] = {
	/* No error	*/	"",
	/* BadRequest	*/	"bad request code",
	/* BadValue	*/	"integer parameter out of range",
	/* BadWindow	*/	"parameter not a Window",
	/* BadPixmap	*/	"parameter not a Pixmap",
	/* BadAtom	*/	"parameter not an Atom",
	/* BadCursor	*/	"parameter not a Cursor",
	/* BadFont	*/	"parameter not a Font",
	/* BadMatch	*/	"parameter mismatch",
	/* BadDrawable	*/	"parameter not a Pixmap or Window",
	/* BadAccess	*/	"attempt to access private resource", 
	/* BadAlloc	*/	"insufficient resources",
    	/* BadColor   	*/  	"no such colormap",
    	/* BadGC   	*/  	"parameter not a GC",
	/* BadIDChoice  */	"invalid resource ID for this connection",
	/* BadName	*/	"font or color name does not exist",
	/* BadLength	*/	"request length incorrect; internal Xlib error",
	/* BadImplementation */	"server does not implement function",
};

XGetErrorText(dpy, code, buffer, nbytes)
    register int code;
    register Display *dpy;
    char *buffer;
    int nbytes;
{

    char *defaultp = NULL;
    char buf[32];
    register _XExtension *ext;

    sprintf(buf, "%d\0", code);

    
    if (code <= (sizeof(XErrorList)/ sizeof (char *)) && code > 0) {
	defaultp =  XErrorList[code];
       XGetErrorDatabaseText(dpy, "XProtoError", buf, defaultp, buffer, nbytes);
	}
    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
 	if (ext->error_string != NULL) 
 	    (*ext->error_string)(dpy, code, &ext->codes, buffer, nbytes);
 	ext = ext->next;
    }    
    return;
}

XGetErrorDatabaseText(dpy, name, type, defaultp, buffer, nbytes)
    register char *name, *type;
    char *defaultp;
    register Display *dpy;
    char *buffer;
    int nbytes;
{
    static XrmResourceDataBase db;
    XrmName namelist[5];
    XrmClass classlist[5];
    XrmValue result;
    static int initialized = False;
    char temp[BUFSIZ];

    if (initialized == False) {
	_XInitErrorHandling (&db);
	initialized = True;
    }
    sprintf(temp, "%s.%s", name, type);
    XrmStringToNameList(temp, namelist);
    XrmStringToClassList("ErrorType.ErrorNumber", classlist);

    XrmGetResource(DefaultScreen(dpy), db,
		namelist, classlist, XrmQString, &result);
    if (result.addr) {
	(void) strncpy (buffer, result.addr, nbytes);
	if (result.size < nbytes) buffer[result.size] = 0;
    } else (void) strncpy(buffer, defaultp, nbytes);
}


_XInitErrorHandling (db)
    XrmResourceDataBase *db;
    {
    XrmResourceDataBase errordb;

    XrmInitialize();

    errordb = XrmGetDataBase(ErrorDataBase);
    XrmMergeDataBases(errordb, db);
    }
