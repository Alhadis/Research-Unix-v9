#ifndef lint
static char *rcsid_Resize_c = "$Header: Resize.c,v 1.18 87/09/09 12:01:04 swick Exp $";
#endif	lint

#include <X11/copyright.h>

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



/*
 * MODIFICATION HISTORY
 *
 * 000 -- M. Gancarz, DEC Ultrix Engineering Group
 * 001 -- Loretta Guarino Reid, DEC Ultrix Engineering Group
 *        Convert to X11
 */

#ifndef lint
static char *sccsid = "@(#)Resize.c	3.8	1/24/86";
#endif

#include "uwm.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define max(a,b) ( (a) > (b) ? (a) : (b) )
#define min(a,b) ( (a) > (b) ? (b) : (a) )
#define makemult(a, b) ((b==1) ? (a) : (((int)((a) / (b))) * (b)) )

Bool Resize(window, mask, button, x0, y0)
Window window;                          /* Event window. */
int mask;                               /* Button/key mask. */
int button;                             /* Button event detail. */
int x0, y0;                             /* Event mouse position. */
{
    XWindowAttributes window_info;	/* Event window info. */
    int x1, y1;                         /* fixed box corner   */
    int x2, y2;                         /* moving box corner   */
    int x, y;
    int xinc, yinc;
    int minwidth, minheight;
    int maxwidth, maxheight;
    int defwidth, defheight;
    int ox, oy;				/* which quadrant of window */
    int pop_x, pop_y;			/* location of pop window */
    int hsize, vsize;			/* dynamic size */
    int delta;				
    int root_x, root_y;			/* root window coordinates */
    int ptrmask;			/* pointer status word */
    int num_vectors;			/* Number of vectors to XDraw. */
    Window assoc;			/* Window represented by the icon. */
    Window sub_win;			/* Mouse query sub window. */
    Window root;			/* Root query window. */
    XEvent button_event;		/* Button event packet. */
    XSegment box[MAX_BOX_VECTORS];	/* Box drawing vertex buffer. */
    XSegment zap[MAX_ZAP_VECTORS];	/* Zap drawing vertex buffer. */
    Bool stop;				/* Should the window stop changing? */
    XSizeHints sizehints;
    XWindowChanges values;
    int width_offset, height_offset;	/* to subtract if resize increments */

    /*
     * Do nothing if the event window is the root window.
     */
    if (window == RootWindow(dpy, scr))
        return(FALSE);

    /*
     * Gather info about the event window.
     */
    status = XGetWindowAttributes(dpy, window, &window_info);
    if (status == FAILURE) return(FALSE);

    /*
     * Do not resize an icon window (NULL for assoc means don't create too). 
     */
    if (IsIcon(window, x, y, FALSE, NULL))
        return(FALSE);

    /*
     * Clear the vector buffers.
     */
    bzero(box, sizeof(box));
    if (Zap) bzero(zap, sizeof(zap));

    /*
     * If we are here then we have a resize operation in progress.
     */

    /*
     * Turn on the resize cursor.
     */
    XChangeActivePointerGrab(dpy, EVENTMASK, ArrowCrossCursor, CurrentTime);

    /*
     * calculate fixed point (x1, y1) and varying point (x2, y2).
     */

    hsize = defwidth = window_info.width; 
    vsize = defheight = window_info.height;
    x1 = window_info.x;
    y1 = window_info.y;
    x2 = x1 + hsize;
    y2 = y1 + vsize;

    /*
     * Get the event window resize hint.
     */
    sizehints.flags = 0;
    XGetSizeHints(dpy, window, &sizehints, XA_WM_NORMAL_HINTS); 
    CheckConsistency(&sizehints);

    /* until there are better WM_HINTS, we'll assume that the client's
     * minimum width and height are the appropriate offsets to subtract
     * when resizing with an explicit resize increment.
     */
    if (sizehints.flags&PMinSize && sizehints.flags&PResizeInc) {
        width_offset = sizehints.min_width;
        height_offset = sizehints.min_height;
    } else
        width_offset = height_offset = 0;

    /*
     * decide what resize mode we are in. Always rubberband if window
     * is too small.
     */
    if (window_info.width > 2 && window_info.height > 2) {
      ox = ((x0 - window_info.x) * 3) / window_info.width;
      oy = ((y0 - window_info.y) * 3) / window_info.height;
      if ((ox + oy) & 1) {
	if (ox & 1) {
	    /* fix up size hints so that we will never change width */
	    sizehints.min_width = sizehints.max_width = window_info.width;
	    if ((sizehints.flags&PMinSize) == 0) {
	      sizehints.min_height = 0;
	      sizehints.flags |= PMinSize;
	    }
	    if ((sizehints.flags&PMaxSize) == 0) {
	      sizehints.max_height = DisplayHeight(dpy, scr);
	      sizehints.flags |= PMaxSize;
	    }
	}
	if (oy & 1) {
	    /* fix up size hints so that we will never change height */
	    sizehints.min_height = sizehints.max_height = window_info.height;
	    if ((sizehints.flags&PMinSize)==0) {
	      sizehints.min_width = 0;
	      sizehints.flags |= PMinSize;
	    }
	    if ((sizehints.flags&PMaxSize)==0) {
	      sizehints.max_width = DisplayWidth(dpy, scr);
	      sizehints.flags |= PMaxSize;
	    }
	}
      }
    }
    else ox = oy = 2;

    /* change fixed point to one that shouldn't move */
    if (oy == 0) { 
	y = y1; y1 = y2; y2 = y;
    }
    if (ox == 0) { 
	x = x1; x1 = x2; x2 = x;
    }


    if (sizehints.flags&PMinSize) {
        minwidth = sizehints.min_width;
        minheight = sizehints.min_height;
    } else {
        minwidth = 0;
        minheight = 0;
    }
    if (sizehints.flags&PMaxSize) {
        maxwidth = max(sizehints.max_width, minwidth);
        maxheight = max(sizehints.max_height, minheight);
    } else {
        maxwidth = DisplayWidth(dpy, scr);
        maxheight = DisplayHeight(dpy, scr);
    }
    if (sizehints.flags&PResizeInc) {
        xinc = sizehints.width_inc;
        yinc = sizehints.height_inc;
    } else {
        xinc = 1;
        yinc = 1;
    }

    switch (ox) {
        case 0: pop_x = x1 - PWidth; break;
        case 1: pop_x = x1 + (hsize-PWidth)/2; break;
        case 2: pop_x = x1; break;
    }
    switch (oy) {
        case 0: pop_y = y1 - PHeight; break;
        case 1: pop_y = y1 + (vsize-PHeight)/2; break;
        case 2: pop_y = y1; break;
     }
    values.x = pop_x;
    values.y = pop_y;
    values.stack_mode = Above;
    XConfigureWindow(dpy, Pop, CWX|CWY|CWStackMode, &values);
    XMapWindow(dpy, Pop);

    if (Grid) {
    	num_vectors = StoreGridBox(
	    box,
	    MIN(x1, x2), MIN(y1, y2),
	    MAX(x1, x2), MAX(y1, y2)
	);
    }
    else {
    	num_vectors = StoreBox(
	    box,
	    MIN(x1, x2), MIN(y1, y2),
	    MAX(x1, x2), MAX(y1, y2)
	);
    }

    /*
     * If we freeze the server, then we will draw solid
     * lines instead of flickering ones during resizing.
     */
    if (Freeze) XGrabServer(dpy);

    /*
     * Process any pending exposure events before drawing the box.
     */
    while (QLength(dpy) > 0) {
        XPeekEvent(dpy, &button_event);
        if (((XAnyEvent *)&button_event)->window == RootWindow(dpy, scr))
            break;
        GetButton(&button_event);
    }

    /*
     * Now draw the box.
     */
    DrawBox();
    Frozen = window;

    stop = FALSE;
    x = -1; y = -1;

    while (!stop) {
	if (x != x2 || y != y2) {
	
	    x = x2; y = y2;
	    
            /*
             * If we've frozen the server, then erase
             * the old box.
             */
            if (Freeze)
                DrawBox();

	    if (Grid) {
	    	num_vectors = StoreGridBox(
		    box,
		    MIN(x1, x), MIN(y1, y),
		    MAX(x1, x), MAX(y1, y)
		);
	    }
	    else {
	    	num_vectors = StoreBox(
		    box,
		    MIN(x1, x), MIN(y1, y),
		    MAX(x1, x), MAX(y1, y)
		);
	    }

            if (Freeze)
                DrawBox();

	    {
	        int Hsize = (hsize - width_offset) / xinc;
		int Vsize = (vsize - height_offset) / yinc;
		int pos = 4;

		PText[0] = (Hsize>99) ? (Hsize / 100 + '0')	  : ' ';
		PText[1] = (Hsize>9)  ? ((Hsize / 10) % 10 + '0') : ' ';
		PText[2] = Hsize % 10 + '0';
		if (Vsize>99) PText[pos++] = Vsize / 100 + '0';
		if (Vsize>9)  PText[pos++] = (Vsize / 10) % 10 + '0';
		PText[pos++] = Vsize % 10 + '0';
		while (pos<7) PText[pos++] = ' ';
	    }

	    /*
	     * If the font is not fixed width we have to
	     * clear the window to guarantee that the characters
	     * that were there before are erased.
	     */
	    if (!(PFontInfo->per_char)) XClearWindow(dpy, Pop);
	    XDrawImageString(
	        dpy, Pop, PopGC,
	        PPadding, PPadding+PFontInfo->ascent,
	        PText, PTextSize);
	}

        if (!Freeze) {
            DrawBox();
            DrawBox();
        }

	if (XPending(dpy) && GetButton(&button_event)) {

            if ((button_event.type != ButtonPress) && 
	        (button_event.type != ButtonRelease)) {
                continue; /* spurious menu event... */
            }

            if (Freeze) {
                DrawBox();
                Frozen = (Window)0;
                XUngrabServer(dpy);
            }

	    if ((button_event.type == ButtonRelease) &&
                (((XButtonReleasedEvent *)&button_event)->button == button)){
		x2 = ((XButtonReleasedEvent *)&button_event)->x;
		y2 = ((XButtonReleasedEvent *)&button_event)->y;
		stop = TRUE;
	    }
	    else {
		XUnmapWindow(dpy, Pop);
		ResetCursor(button);
		return(TRUE);
	    }
	}
	else {
	    XQueryPointer(dpy, RootWindow(dpy, scr), &root, 
	    		&sub_win, &root_x, &root_y, &x2, &y2, &ptrmask);
	}


	hsize = max(min(abs (x2 - x1), maxwidth), minwidth);
	hsize = makemult(hsize-minwidth, xinc)+minwidth;
 
	vsize = max(min(abs(y2 - y1), maxheight), minheight);
	vsize = makemult(vsize-minheight, yinc)+minheight; 

	if (sizehints.flags & PAspect) {
            if ((hsize * sizehints.max_aspect.y > 
	          vsize * sizehints.max_aspect.x)) {
	       delta = makemult( 
			 (hsize*sizehints.max_aspect.y /
		         sizehints.max_aspect.x)
			  - vsize, 
		       yinc); 
	       if ((vsize+delta <= maxheight))  vsize += delta;
	       else {
	         delta = makemult(hsize - 
		     (sizehints.max_aspect.x * vsize/sizehints.max_aspect.y), 
		     xinc);
		 if (hsize-delta >= minwidth) hsize -= delta; 
	       }
            }  
            if (hsize * sizehints.min_aspect.y < vsize * 
		    sizehints.min_aspect.x) {
	       delta = makemult( 
		        (sizehints.min_aspect.x * 
			  vsize/sizehints.min_aspect.y) - hsize, 
	 	        xinc);
	       if (hsize+delta <= maxwidth) hsize += delta;
	       else {
	         delta = makemult(
		       vsize - 
		         (hsize*sizehints.min_aspect.y /
			 sizehints.min_aspect.x), 
		       yinc); 
	         if ((vsize-delta >= minheight))  vsize -= delta; 
	       }
	    }
 		  
      }
      if (ox == 0)
 	x2 = x1 - hsize;
      else
	x2 = x1 + hsize;

      if (oy == 0)
	y2 = y1 - vsize;
      else
    	y2 = y1 + vsize;
	    
    }
    if (x2 < x1) {
       x = x1; x1 = x2; x2 = x;
    }
    if (y2 < y1) {
        y = y1; y1 = y2; y2 = y;
    }
    XUnmapWindow(dpy, Pop);
    if ((x1!=window_info.x) || (y1 != window_info.y) || 
        (hsize != window_info.width) ||
        (vsize != window_info.height))
        XMoveResizeWindow(dpy, window, x1, y1, hsize, vsize);
    return(TRUE);
}

CheckConsistency(hints)
XSizeHints *hints;
{
  if (hints->min_height < 0) hints->min_height = 0;
  if (hints->min_width < 0)  hints->min_width = 0;

  if (hints->max_height <= 0 || hints->max_width <= 0)
      hints->flags &= ~PMaxSize;

  hints->min_height = min(DisplayHeight(dpy, scr), hints->min_height);
  hints->min_width =  min(DisplayWidth(dpy, scr),  hints->min_width);

  hints->max_height = min(DisplayHeight(dpy, scr), hints->max_height);
  hints->max_width =  min(DisplayWidth(dpy, scr),  hints->max_width);

  if ((hints->flags&PMinSize) && (hints->flags&PMaxSize) && 
   ((hints->min_height > hints->max_height) ||
    (hints->min_width > hints->max_width)))
	hints->flags &= ~(PMinSize|PMaxSize);

  if ((hints->flags&PAspect) && 
   (hints->min_aspect.x * hints->max_aspect.y > 
     hints->max_aspect.x * hints->min_aspect.y))
	hints->flags &= ~(PAspect);
}
