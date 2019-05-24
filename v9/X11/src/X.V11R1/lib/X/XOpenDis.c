#include "copyright.h"
#ifndef lint
static char *rcsid_xopendisplay_c = "$Header: XOpenDis.c,v 11.50 87/09/01 15:00:36 toddb Exp $";
#endif
/* Copyright    Massachusetts Institute of Technology    1985, 1986	*/

/* Converted to V11 by jg */
 
#include <stdio.h>
#include "Xlibint.h"
#include <strings.h>
#include "Xatom.h"

#ifndef lint
static int lock;	/* get rid of ifdefs when locking implemented */
#endif

int _Xdebug = 0;
static xReq _dummy_request = {
	0, 0, 0
};

/* head of the linked list of open displays */
Display *_XHeadOfDisplayList = NULL;

extern _XWireToEvent();
extern _XUnknownNativeEvent();
extern _XUnknownWireEvent();
/* 
 * Connects to a server, creates a Display object and returns a pointer to
 * the newly created Display back to the caller.
 */
Display *XOpenDisplay (display)
	register char *display;
{
	register Display *dpy;		/* New Display object being created. */
	register int i;
	int j, k;			/* random iterator indexes */
	char *display_name;		/* pointer to display name */
	int indian;			/* to determine which indian. */
	xConnClientPrefix client;	/* client information */
	xConnSetupPrefix prefix;	/* prefix information */
	int vendorlen;			/* length of vendor string */
	char *setup;			/* memory allocated at startup */
	char displaybuf[256];		/* buffer to receive expanded name */
	int screen_num;			/* screen number */
	union {
		xConnSetup *setup;
		char *failure;
		char *vendor;
		xPixmapFormat *sf;
		xWindowRoot *rp;
		xDepth *dp;
		xVisualType *vp;
	} u;
	long setuplength;	/* number of bytes in setup message */

	extern int _XSendClientPrefix();
	extern int _XConnectDisplay();
	extern char *getenv();
	extern XID _XAllocID();
 
	/*
	 * If the display specifier string supplied as an argument to this 
	 * routine is NULL or a pointer to NULL, read the DISPLAY variable.
	 */
	if (display == NULL || *display == '\0') {
		if ((display_name = getenv("DISPLAY")) == NULL) {
			/* Oops! No DISPLAY environment variable - error. */
			return(NULL);
		}
	}
	else {
		/* Display is non-NULL, copy the pointer */
		display_name = display;
	}

/*
 * Attempt to allocate a display structure. Return NULL if allocation fails.
 */
	LockMutex(&lock);
	if ((dpy = (Display *)Xcalloc(1, sizeof(Display))) == NULL) {
		errno = ENOMEM;
		UnlockMutex(&lock);
		return(NULL);
	}

/*
 * Call the Connect routine to get the network socket. If 0 is returned, the
 * connection failed. The connect routine will return the expanded display
 * name in displaybuf.
 */

	if ((dpy->fd = _XConnectDisplay(display_name, displaybuf, &screen_num))
	     < 0) {
		Xfree ((char *) dpy);
		UnlockMutex(&lock);
		return(NULL);		/* errno set by XConnectDisplay */
	}

/*
 * First byte is the byte order byte.
 * Authentication key is normally sent right after the connection.
 * This (in MIT's case) will be kerberos.
 */
	indian = 1;
	if (*(char *) &indian)
	    client.byteOrder = 'l';
	else
	    client.byteOrder = 'B';
	client.majorVersion = X_PROTOCOL;
	client.minorVersion = X_PROTOCOL_REVISION;
	_XSendClientPrefix (dpy, &client);
/*
 * Now see if connection was accepted...
 */
	_XRead (dpy, (char *)&prefix,(long)sizeof(xConnSetupPrefix));

	if (prefix.majorVersion < X_PROTOCOL) {
		(void) fputs ("Warning: Client built for newer server!\n", stderr);
	}
	if (prefix.minorVersion != X_PROTOCOL_REVISION) {
		(void) fputs (
		 "Warning: Protocol rev. of client does not match server!\n",
		  stderr);
	}

	setuplength = prefix.length << 2;
	if ( (u.setup = (xConnSetup *)(setup =  Xmalloc ((unsigned)setuplength)))
	    == NULL) {
		errno = ENOMEM;
		Xfree ((char *)dpy);
		UnlockMutex(&lock);
		return(NULL);
	}
	_XRead (dpy, (char *)u.setup, setuplength);
/*
 * If the connection was not accepted by the server due to problems,
 * give error message to the user....
 */
	if (prefix.success != xTrue) {
/*
		(void) fwrite (u.failure, (int)*u.failure, sizeof(char), stderr);
*/
		(void) fwrite (u.failure, sizeof(char),
			(int)prefix.lengthReason, stderr);
		(void) fwrite ("\n", sizeof(char), 1, stderr);
		Xfree ((char *)dpy);
		Xfree (setup);
		UnlockMutex(&lock);
		return (NULL);
	}

/*
 * We succeeded at authorization, so let us move the data into
 * the display structure.
 */
	dpy->next		= (Display *) NULL;
	dpy->proto_major_version= prefix.majorVersion;
	dpy->proto_minor_version= prefix.minorVersion;
	dpy->release 		= u.setup->release;
	dpy->resource_base	= u.setup->ridBase;
	dpy->resource_mask	= u.setup->ridMask;
	dpy->min_keycode	= u.setup->minKeyCode;
	dpy->max_keycode	= u.setup->maxKeyCode;
	dpy->keysyms		= (KeySym *) NULL;
	dpy->modifiermap	= XNewModifiermap(0);
	dpy->keysyms_per_keycode = 0;
	dpy->current		= None;
	dpy->xdefaults		= (char *)NULL;
	dpy->scratch_length	= 0L;
	dpy->scratch_buffer	= NULL;
	dpy->motion_buffer	= u.setup->motionBufferSize;
	dpy->nformats		= u.setup->numFormats;
	dpy->nscreens		= u.setup->numRoots;
	dpy->byte_order		= u.setup->imageByteOrder;
	dpy->bitmap_unit	= u.setup->bitmapScanlineUnit;
	dpy->bitmap_pad		= u.setup->bitmapScanlinePad;
	dpy->bitmap_bit_order   = u.setup->bitmapBitOrder;
	dpy->max_request_size	= u.setup->maxRequestSize;
	dpy->ext_procs		= (_XExtension *)NULL;
	dpy->ext_data		= (XExtData *)NULL;
	dpy->ext_number 	= 0;
	dpy->event_vec[X_Error] = _XUnknownWireEvent;
	dpy->event_vec[X_Reply] = _XUnknownWireEvent;
	dpy->wire_vec[X_Error]  = _XUnknownNativeEvent;
	dpy->wire_vec[X_Reply]  = _XUnknownNativeEvent;
	for (i = KeyPress; i < LASTEvent; i++) {
	    dpy->event_vec[i] 	= _XWireToEvent;
	    dpy->wire_vec[i] 	= NULL;
	}
	for (i = LASTEvent; i < 128; i++) {
	    dpy->event_vec[i] 	= _XUnknownWireEvent;
	    dpy->wire_vec[i] 	= _XUnknownNativeEvent;
	}
	dpy->resource_id	= 0;
	dpy->resource_shift	= ffs(dpy->resource_mask) - 1;
	dpy->db 		= (struct _XrmResourceDataBase *)NULL;
/* 
 * Initialize pointers to NULL so that XFreeDisplayStructure will
 * work if we run out of memory
 */

	dpy->screens = NULL;
	dpy->display_name = NULL;
	dpy->buffer = NULL;

/*
 * now extract the vendor string...  String must be null terminated,
 * padded to multiple of 4 bytes.
 */
	dpy->vendor = (char *) Xmalloc (u.setup->nbytesVendor + 1);
	vendorlen = u.setup->nbytesVendor;
	u.setup += 1;	/* can't touch information in XConnSetup anymore..*/
	(void) strncpy(dpy->vendor, u.vendor, vendorlen);
	u.vendor += (vendorlen + 3) & ~3;
/*
 * Now iterate down setup information.....
 */
	dpy->pixmap_format = 
	    (ScreenFormat *)Xmalloc(
		(unsigned) (dpy->nformats *sizeof(ScreenFormat)));
	if (dpy->pixmap_format == NULL) {
	        OutOfMemory (dpy, setup);
		UnlockMutex(&lock);
		return(NULL);
	}
/*
 * First decode the Z axis Screen format information.
 */
	for (i = 0; i < dpy->nformats; i++) {
	    register ScreenFormat *fmt = &dpy->pixmap_format[i];
	    fmt->depth = u.sf->depth;
	    fmt->bits_per_pixel = u.sf->bitsPerPixel;
	    fmt->scanline_pad = u.sf->scanLinePad;
	    fmt->ext_data = NULL;
	    u.sf += 1;
	}

/*
 * next the Screen structures.
 */
	dpy->screens = 
	    (Screen *)Xmalloc((unsigned) dpy->nscreens*sizeof(Screen));
	if (dpy->screens == NULL) {
	        OutOfMemory (dpy, setup);
		UnlockMutex(&lock);
		return(NULL);
	}
/*
 * Now go deal with each screen structure.
 */
	for (i = 0; i < dpy->nscreens; i++) {
	    register Screen *sp = &dpy->screens[i];
	    VisualID root_visualID = u.rp->rootVisualID;
	    sp->display	    = dpy;
	    sp->root 	    = u.rp->windowId;
	    sp->cmap 	    = u.rp->defaultColormap;
	    sp->white_pixel = u.rp->whitePixel;
	    sp->black_pixel = u.rp->blackPixel;
	    sp->root_input_mask = u.rp->currentInputMask;
	    sp->width	    = u.rp->pixWidth;
	    sp->height	    = u.rp->pixHeight;
	    sp->mwidth	    = u.rp->mmWidth;
	    sp->mheight	    = u.rp->mmHeight;
	    sp->min_maps    = u.rp->minInstalledMaps;
	    sp->max_maps    = u.rp->maxInstalledMaps;
	    sp->root_visual = NULL;  /* filled in later, when we alloc Visuals */
	    sp->backing_store= u.rp->backingStore;
	    sp->save_unders = u.rp->saveUnders;
	    sp->root_depth  = u.rp->rootDepth;
	    sp->ndepths	    = u.rp->nDepths;
	    sp->ext_data   = NULL;
	    u.rp += 1;
/*
 * lets set up the depth structures.
 */
	    sp->depths = (Depth *)Xmalloc(
			(unsigned)sp->ndepths*sizeof(Depth));
	    if (sp->depths == NULL) {
		OutOfMemory (dpy, setup);
		UnlockMutex(&lock);
		return(NULL);
	    }
	    /*
	     * for all depths on this screen.
	     */
	    for (j = 0; j < sp->ndepths; j++) {
		Depth *dp = &sp->depths[j];
		dp->depth = u.dp->depth;
		dp->nvisuals = u.dp->nVisuals;
		u.dp += 1;
		dp->visuals = 
		  (Visual *)Xmalloc((unsigned)dp->nvisuals*sizeof(Visual));
		if (dp->visuals == NULL) {
		    OutOfMemory (dpy, setup);
		    UnlockMutex(&lock);
		    return(NULL);
		}
		for (k = 0; k < dp->nvisuals; k++) {
			register Visual *vp = &dp->visuals[k];
			if ((vp->visualid = u.vp->visualID) == root_visualID)
			   sp->root_visual = vp;
			vp->class	= u.vp->class;
			vp->bits_per_rgb= u.vp->bitsPerRGB;
			vp->map_entries	= u.vp->colormapEntries;
			vp->red_mask	= u.vp->redMask;
			vp->green_mask	= u.vp->greenMask;
			vp->blue_mask	= u.vp->blueMask;
			vp->ext_data	= NULL;
			u.vp += 1;
		}
	    }
	}
		

/*
 * Setup other information in this display structure.
 */
	dpy->vnumber = X_PROTOCOL;
	dpy->resource_alloc = _XAllocID;
	dpy->synchandler = NULL;
	dpy->request = 0;
	dpy->last_request_read = 0;
	dpy->default_screen = screen_num;  /* Value returned by ConnectDisplay */
	dpy->last_req = (char *)&_dummy_request;

	/* Salt away the host:display string for later use */
	if ((dpy->display_name = Xmalloc(
		(unsigned) (strlen(displaybuf) + 1))) == NULL) {
	        OutOfMemory (dpy, setup);
		UnlockMutex(&lock);
		return(NULL);
	}
	(void) strcpy (dpy->display_name, displaybuf);
 
	/* Set up the output buffers. */
	if ((dpy->bufptr = dpy->buffer = Xmalloc(BUFSIZE)) == NULL) {
	        OutOfMemory (dpy, setup);
		UnlockMutex(&lock);
		return(NULL);
	}
	dpy->bufmax = dpy->buffer + BUFSIZE;
 
	/* Set up the input event queue and input event queue parameters. */
	dpy->head = dpy->tail = NULL;
	dpy->qlen = 0;
 
/*
 * Now start talking to the server to setup all other information...
 */

	Xfree (setup);	/* all finished with setup information */
/*
 * Set up other stuff clients are always going to use.
 */
	for (i = 0; i < dpy->nscreens; i++) {
	    register Screen *sp = &dpy->screens[i];
	    XGCValues values;
	    values.foreground = sp->white_pixel;
	    values.background = sp->black_pixel;
	    sp->default_gc = XCreateGC (dpy, sp->root,
			GCForeground|GCBackground, &values);
	}
/*
 * call into synchronization routine so that all programs can be
 * forced synchronize
 */
	(void) XSynchronize(dpy, _Xdebug);
/*
 * chain this stucture onto global list.
 */
	dpy->next = _XHeadOfDisplayList;
	_XHeadOfDisplayList = dpy;
	UnlockDisplay(dpy);
	UnlockMutex(&lock);
/*
 * get the resource manager database off the root window.
 */
	{
	    Atom actual_type;
	    int actual_format;
	    unsigned long nitems;
	    long leftover;
	    if (XGetWindowProperty(dpy, RootWindow(dpy, 0), 
		XA_RESOURCE_MANAGER, 0L, 100000000L, False, XA_STRING,
		&actual_type, &actual_format, &nitems, &leftover, 
		&dpy->xdefaults) != Success) {
			dpy->xdefaults = (char *) NULL;
		}
	    else {
	    if ( (actual_type != XA_STRING) ||  (actual_format != 8) ) {
		if (dpy->xdefaults != NULL) Xfree ( dpy->xdefaults );
		}
	    }
	}
 	return(dpy);
}


/* OutOfMemory is called if malloc fails.  XOpenDisplay returns NULL
   after this returns. */

static OutOfMemory (dpy, setup)
    Display *dpy;
    char *setup;
    {
    _XDisconnectDisplay (dpy->fd);
    _XFreeDisplayStructure (dpy);
    Xfree (setup);
    errno = ENOMEM;
    }


/* XFreeDisplayStructure frees all the storage associated with a 
 * Display.  It is used by XOpenDisplay if it runs out of memory,
 * and also by XCloseDisplay.   It needs to check whether all pointers
 * are non-NULL before dereferencing them, since it may be called
 * by XOpenDisplay before the Display structure is fully formed.
 * XOpenDisplay must be sure to initialize all the pointers to NULL
 * before the first possible call on this.
 */

_XFreeDisplayStructure(dpy)
	register Display *dpy;
{
	if (dpy->screens) {
	    register int i;

            for (i = 0; i < dpy->nscreens; i++) {
		Screen *sp = &dpy->screens[i];

		if (sp->depths) {
		   register int j;

		   for (j = 0; j < sp->ndepths; j++) {
			Depth *dp = &sp->depths[j];

			if (dp->visuals) {
			   register int k;

			   for (k = 0; k < dp->nvisuals; k++)
			     _XFreeExtData (dp->visuals[k].ext_data);
			   Xfree ((char *) dp->visuals);
			   }
			}

		   Xfree ((char *) sp->depths);
		   }

		_XFreeExtData (sp->ext_data);
		}

	    Xfree ((char *)dpy->screens);
	    }
	
	if (dpy->pixmap_format) {
	    register int i;

	    for (i = 0; i < dpy->nformats; i++)
	      _XFreeExtData (dpy->pixmap_format[i].ext_data);
            Xfree ((char *)dpy->pixmap_format);
	    }

	if (dpy->display_name)
	   Xfree (dpy->display_name);

        if (dpy->buffer)
	   Xfree (dpy->buffer);
	if (dpy->keysyms)
	   Xfree ((char *) dpy->keysyms);
	if (dpy->xdefaults)
	   Xfree (dpy->xdefaults);

	_XFreeExtData (dpy->ext_data);
        
	Xfree ((char *)dpy);
}
