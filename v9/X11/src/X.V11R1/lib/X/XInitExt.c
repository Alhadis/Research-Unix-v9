#include "copyright.h"

/* $Header: XInitExt.c,v 11.13 87/08/28 13:35:22 toddb Exp $ */
/* Copyright  Massachusetts Institute of Technology 1987 */

#include "Xlibint.h"
#include <stdio.h>

extern _XUnknownWireEvent();
extern _XUnknownNativeEvent();

/*
 * This routine is used to link a extension in so it will be called
 * at appropriate times.
 */

XExtCodes *XInitExtension (dpy, name)
	Display *dpy;
	char *name;
{
	XExtCodes codes;	/* temp. place for extension information. */
	register _XExtension *ext;/* need a place to build it all */
	if (!XQueryExtension(dpy, name, 
		&codes.major_opcode, &codes.first_event,
		&codes.first_error)) return (NULL);

	LockDisplay (dpy);
	ext = (_XExtension *) Xcalloc (1, sizeof (_XExtension));
	codes.extension = dpy->ext_number++;
	ext->codes = codes;
	
	/* chain it onto the display list */
	ext->next = dpy->ext_procs;
	dpy->ext_procs = ext;
	UnlockDisplay (dpy);

	return (&ext->codes);		/* tell him which extension */
}

static _XExtension *XLookupExtension (dpy, extension)
	register Display *dpy;	/* display */
	register int extension;	/* extension number */
{
	register _XExtension *ext = dpy->ext_procs;
	while (ext != NULL) {
		if (ext->codes.extension == extension) return (ext);
		ext = ext->next;
	}
	return (NULL);
}

/*
 * Routines to hang procs on the extension structure.
 */
int (*XESetCreateGC(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when GC created */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->create_GC;
	e->create_GC = proc;

	return (oldproc);
}
int (*XESetCopyGC(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when GC copied */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->copy_GC;
	e->copy_GC = proc;

	return (oldproc);
}
int (*XESetFlushGC(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when GC copied */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->flush_GC;
	e->flush_GC = proc;

	return (oldproc);
}

int (*XESetFreeGC(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when GC freed */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->free_GC;
	e->free_GC = proc;

	return (oldproc);
}

int (*XESetCreateFont(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when font created */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->create_Font;
	e->create_Font = proc;

	return (oldproc);
}

int (*XESetFreeFont(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when font freed */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->free_Font;
	e->free_Font = proc;

	return (oldproc);
}

int (*XESetCloseDisplay(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when display closed */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->close_display;
	e->close_display = proc;

	return (oldproc);
}
int (*XESetWireToEvent(dpy, event_number, proc))()
	Display *dpy;		/* display */
	int (*proc)();		/* routine to call when converting event */
	int event_number;	/* event routine to replace */
{
	register int (*oldproc)();
	if (proc == NULL) proc = _XUnknownWireEvent;
	LockDisplay (dpy);
	oldproc = dpy->event_vec[event_number];
	dpy->event_vec[event_number] = proc;
	UnlockDisplay (dpy);

	return (oldproc);
}
int (*XESetEventToWire(dpy, event_number, proc))()
	Display *dpy;		/* display */
	int (*proc)();		/* routine to call when converting event */
	int event_number;	/* event routine to replace */
{
	register int (*oldproc)();
	if (proc == NULL) proc = _XUnknownNativeEvent;
	LockDisplay (dpy);
	oldproc = dpy->wire_vec[event_number];
	dpy->wire_vec[event_number] = proc;
	return (oldproc);
}
int (*XESetError(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when X error happens */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->error;
	e->error = proc;

	return (oldproc);
}
int (*XESetErrorString(dpy, extension, proc))()
	Display *dpy;		/* display */
	int extension;		/* extension number */
	int (*proc)();		/* routine to call when I/O error happens */
{
	register _XExtension *e;	/* for lookup of extension */
	register int (*oldproc)();
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);

	oldproc = e->error_string;
	e->error_string = proc;

	return (oldproc);
}
