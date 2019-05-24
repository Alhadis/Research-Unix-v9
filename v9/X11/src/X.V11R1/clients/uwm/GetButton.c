#ifndef lint
static char *rcsid_GetButton_c = "$Header: GetButton.c,v 1.24 87/09/09 19:20:45 swick Exp $";
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
 * 001 -- L. Guarino Reid, DEC Ultrix Engineering Group, Western Software Lab
 *	  February 16, 1987
 *	  Add EnterWindow, LeaveWindow, and MouseMotion as recognized
 *	  uwm buttons for uwm menus. Add bug fixes to prevent mem faults
 *	  if icon_str is NULL.
 * 002 -- L. Guarino Reid, DEC Ultrix Engineering Group
 *	  April 16, 1987
 *	  Convert to X11
 */

#ifndef lint
static char *sccsid = "@(#)GetButton.c	3.8	1/24/86";
#endif
/*
 *	GetButton - This subroutine is used by the Ultrix Window Manager (uwm)
 *	to acquire button events.  It waits for a button event to occur
 *	and handles all event traffic in the interim.
 *
 *	File:		GetButton.c
 */

#include "uwm.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define ICONSTR	(icon_str ? icon_str : "")

Bool GetButton(button_event)
    XEvent *button_event;	/* Button event packet. */
{
#define STRLEN 50
    XKeyPressedEvent *kp_event;	/* Key pressed event. */
    char *icon_str;		/* Icon's name string. */
    register int icon_str_len;	/* Icon name string lenght.  */
    register int key_char;	/* Key press character code. */
    register int icon_x;	/* Icon window X coordinate. */
    register int icon_y;	/* Icon window Y coordinate. */
    register int icon_w;	/* Icon window width. */
    register int icon_h;	/* Icon window height. */    
    int status;			/* Routine call return status. */
    Window icon;		/* Icon window. */
    Window appl;		/* Application window. */
    XWindowAttributes icon_info;	/* Icon window info structure. */
    char kbd_str[STRLEN];              /* Keyboard string. */
    int nbytes;                 /* Keyboard string length. */
    int i;                      /* Iteration counter. */


    /*
     * Get next event from input queue and store it in the event packet
     * passed to GetButton.
     */
    XNextEvent(dpy, button_event);

    /*
     * The event occured on the root window, check for substructure
     * changes. Otherwise, it must be a mouse button event. 
     */
    if (((XAnyEvent *)button_event)->window == RootWindow(dpy, scr)) {

	switch (button_event->type) {

	  case CreateNotify:
	  case UnmapNotify:
	  case ReparentNotify:
	  case ConfigureNotify:
	  case GravityNotify:
	  case MapNotify:
	  case MappingNotify:
	  case CirculateNotify: return(FALSE);
	  	
	  case MapRequest: 
	        CheckMap(((XMapEvent *)button_event)->window);
		return(FALSE);

	  case ConfigureRequest: 
	        Configure((XConfigureEvent *)button_event);
		return(FALSE);

	  case CirculateRequest: 
	        Circulate((XCirculateEvent *)button_event);
		return(FALSE);

	  case DestroyNotify: 
	        RemoveIcon(((XDestroyWindowEvent *)button_event)->window);
		return(FALSE);

	  case FocusIn: 
		if (((XFocusInEvent *)button_event)->detail
		    == NotifyPointerRoot) {
    	            if (FocusSetByUser) {
       	                XSetInputFocus(dpy, PointerRoot, None, CurrentTime);
                        FocusSetByUser = FALSE;
		    }
		}
		return (FALSE);

          case FocusOut:
		if (((XFocusOutEvent *)button_event)->detail
		    == NotifyPointerRoot) {
                    if (!FocusSetByUser) {
   	                XSetInputFocus(dpy, PointerRoot, None, CurrentTime);
		    }
		}
		return (FALSE);

	  case ButtonPress:
	  case ButtonRelease:
		return(TRUE);

	  default: 
	    printf("uwm internal error: unexpected event on Root Window\n");
	    return(FALSE); 
	}
    }

    /*
     * If the event type is EnterWindow, LeaveWindow, or MouseMoved,
     * we are processing a menu. 
     * If the event type is ButtonPress or ButtonRelease,
     * we have a button event.      */
    switch (button_event->type) {
       case EnterNotify:
       case LeaveNotify: 
       case MotionNotify: 
       case ButtonPress: 
       case ButtonRelease: 
	return(TRUE); 
       default: break;
    }

    /*
     * Ok, if the event is not on the root window it must be an event on
     * one of the icons owned by uwm.
     */
    icon = ((XAnyEvent *)button_event)->window;

    /*
     * Find out current information about the icon window.
     */
    status = XGetWindowAttributes(dpy, icon, &icon_info);
    if (status == FAILURE) return(FALSE);

    /*
     * If the event is an UnmapWindow event or a ConfigureNotify event,
     * then return FALSE.
     */
    if (button_event->type == MapNotify || 
        button_event->type == UnmapNotify ||
        button_event->type == CreateNotify ||
        button_event->type == ReparentNotify ||
        button_event->type == GravityNotify ||
        button_event->type == CirculateNotify ||
        button_event->type == ConfigureNotify)
        return(FALSE);

    /*
     * Initialize the icon position variables.
     */
    icon_x = icon_info.x;
    icon_y = icon_info.y;

    /*
     * Get the name of the window associated with the icon and
     * determine its length.
     */
    if (!IsIcon(icon, icon_x, icon_y, FALSE, &appl)) return(FALSE);
    icon_str = GetIconName(appl);
    icon_str_len = icon_str ? strlen(icon_str) : 0;

    /*
     * If the event is a window exposure event and the icon's name string
     * is not of zero length, simply repaint the text in the icon window
     * and return FALSE.
     */
    if (button_event->type == Expose && (!Freeze || Frozen == 0)) {
        if (icon_info.width != 
	  XTextWidth(IFontInfo, ICONSTR, strlen(ICONSTR))+(HIconPad << 1)) {
	  XResizeWindow(dpy, icon, 
	    XTextWidth(IFontInfo, ICONSTR, strlen(ICONSTR))+(HIconPad << 1),
	    IFontInfo->ascent + IFontInfo->descent + (VIconPad << 1));
	}
	XClearWindow(dpy, icon);
        if (icon_str_len != 0) {
            XDrawImageString(dpy, icon,
                     IconGC, HIconPad, VIconPad+IFontInfo->ascent,
                     icon_str, icon_str_len);
	    /*
	     * Remember to free the icon name string.
	     */
	    free(icon_str);
        }
	return(FALSE);
    }

    /*
     * If we have gotten this far event can only be a key pressed event.
     */
    kp_event = (XKeyPressedEvent *) button_event;

    /* 
     * We convert the key pressed event to ascii.
     */
    nbytes = XLookupString(kp_event, kbd_str, STRLEN, NULL);

    /*
     * If kbd_str is a "non-string", then don't do anything.
     */
    if (nbytes == 0) {
        if (icon_str) free(icon_str);
        return(FALSE);
    }
    for (i = 0; i < nbytes; i++) {
        key_char = kbd_str[i];
        /*
         * If the key was <DELETE>, then delete a character from the end of
         * the name, return FALSE.
         *
         * If the key was <CTRL-U>, then wipe out the entire window name
         * and return FALSE.
         *
         * All other ctrl keys are squashed and we return FALSE.
         *
         * All printable characters are appended to the window's name, which
         * may have to be grown to allow for the extra length.
         */
        if (key_char == '\177') {
            /*
             * <DELETE>
             */
            if (icon_str_len > 0) {
		icon_str_len--;
		icon_str[icon_str_len] = '\0';
	    }
        }
        else if (key_char == '\025') {
            /*
             * <CTRL-U>
             */
            if (icon_str_len > 0) {
		icon_str_len = 0;
		icon_str[0] = '\0';
	    }
        }
        else if (key_char < IFontInfo->min_char_or_byte2 ||
                 key_char > IFontInfo->max_char_or_byte2) {
            /*
             * Any other random (non-printable) key; ignore it.
             */
	    /* do nothing */ ;
        }
        else {
            /*
             * ASCII Alphanumerics.
             */
	    if (icon_str == NULL)
	    	icon_str = (char *) malloc (icon_str_len + 2);
	    else
	    	icon_str = (char *)realloc(icon_str, (icon_str_len + 2));
            if (icon_str == NULL) {
                errno = ENOMEM;
                Error("GetButton -> Realloc of window name string memory failed.");
            }
            icon_str[icon_str_len] = key_char;
            icon_str[icon_str_len + 1] = '\0';
            icon_str_len += 1;
        }
    }

    /*
     * Now that we have changed the size of the icon we have to reconfigure
     * it so that everything looks good.  Oh yes, don't forget to move the
     * mouse so that it stays in the window!
     */

    /*
     * Set the window name to the new string.
     */
    XSetIconName(dpy, appl, ICONSTR);

    /*
     * Determine the new icon window configuration.
     */
    icon_h = IFontInfo->ascent + IFontInfo->descent + (VIconPad << 1);
    icon_w = XTextWidth(IFontInfo, ICONSTR, strlen(ICONSTR));
    if (icon_w == 0) {
        icon_w = icon_h;
    }
    else {
	icon_w += (HIconPad << 1);
    }

    if (icon_x < 0) icon_x = 0;
    if (icon_y < 0) icon_y = 0;
    if (icon_x - 1 + icon_w + (IBorderWidth << 1) > ScreenWidth) {
	icon_x = ScreenWidth - icon_w - (IBorderWidth << 1) + 1;
    }
    if (icon_y - 1 + icon_h + (IBorderWidth << 1) > ScreenHeight) {
	icon_y = ScreenHeight - icon_h - (IBorderWidth << 1) + 1;
    }

    XMoveResizeWindow(dpy, icon, icon_x, icon_y, icon_w, icon_h);
    XWarpPointer(dpy, None, icon, 
    	0, 0, 0, 0, (icon_w >> 1), (icon_h >> 1));

    /* 
     * Free the local storage and return FALSE.
     */
    if (icon_str) free(icon_str);
    return(FALSE);
}

CheckMap(window)
Window window;
{
    XSizeHints sizehints;
    XWMHints *wmhints;
    int x, y, w, h;
    XWMHints *XGetWMHints();
    Window transient_for;
    Bool configureit = False;
    Window jW;
    int border_width, j;

    /*
     * Gather info about the event window.
     */

    /* if it's a transient window, we won't rubber-band
     * note that this call always sets transient_for.
     */
    if (XGetTransientForHint( dpy, window, &transient_for )) {
        XGetGeometry( dpy, window, &jW, &x, &y, &w, &h, &border_width, &j );
    }
    else {
	if ((wmhints = XGetWMHints(dpy, window)) &&
	    (wmhints->flags&StateHint) &&
	    (wmhints->initial_state == IconicState)) {
	    /* window will remain created size -- no rubberbanding */
	    /* note that Iconify only uses its first argument */
	    Iconify(window, 0, 0, 0, 0);
	    return;
	}

	sizehints.flags = 0; 

        XGetSizeHints(dpy, window, &sizehints, XA_WM_NORMAL_HINTS);
	CheckConsistency(&sizehints);
	AskUser(dpy, scr, window, &x, &y, &w, &h, &sizehints);
	if (x != sizehints.x || y != sizehints.y ||
	    w != sizehints.width || h != sizehints.height)
	  configureit = True;

	sizehints.flags |= (USPosition | USSize);
	sizehints.x = x;
	sizehints.y = y;
	sizehints.width = w;
	sizehints.height = h;
	XSetSizeHints(dpy, window, &sizehints, XA_WM_NORMAL_HINTS);
    }

    if (x<0 || y<0) {
      if (transient_for == None) /* need the border width */
	  XGetGeometry( dpy, window, &jW, &j, &j, &j, &j, &border_width, &j );

      if (x<0) x += DisplayWidth(dpy, scr) - w - (border_width<<1);
      if (y<0) y += DisplayHeight(dpy, scr) - h - (border_width<<1);

      configureit = True;
    }

    if (configureit)
      XMoveResizeWindow(dpy, window, x, y, w, h);

    XMapRaised(dpy, window);
}

Configure(event)
XConfigureRequestEvent *event;
{
  XWindowChanges values;
  
  values.x = event->x;
  values.y = event->y;
  values.width = event->width;
  values.height = event->height;
  values.border_width = event->border_width;
  values.stack_mode = event->detail;
  values.sibling = event->above;

  XConfigureWindow(event->display, event->window, event->value_mask, &values);
}

Circulate(event)
XCirculateEvent *event;
{
  if (event->place == PlaceOnTop)
   XRaiseWindow(event->display, event->window);
  else
   XLowerWindow(event->display, event->window);
}

