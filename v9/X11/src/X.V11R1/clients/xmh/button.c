#ifndef lint
static char rcs_id[] = "$Header: button.c,v 1.10 87/09/11 08:17:57 toddb Exp $";
#endif lint
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

/* button.c -- Handle a button being pressed */

#include "xmh.h"
#include "bboxint.h"


/* Highlight or unhighlight the given button. */

FlipColors(button)
Button button;
{
    static Arg arglist[] = {
	{XtNforeground, NULL},
	{XtNbackground, NULL},
    };
    XtArgVal temp;
    XtCommandGetValues(DISPLAY button->window, arglist, XtNumber(arglist));
    temp = arglist[0].value;
    arglist[0].value = arglist[1].value;
    arglist[1].value = temp;
    XtCommandSetValues(DISPLAY button->window, arglist, XtNumber(arglist));
}


/* The given button has just been pressed.  (This routine is usually called
   by the command button widget code.)  If it's a radio button, select it.
   Then call the function registered for this button. */

void DoButtonPress(button)
Button button;
{
    ButtonBox buttonbox = button->buttonbox;
    Scrn scrn = buttonbox->scrn;
    if (!button->enabled) return;
    if (buttonbox->radio && *(buttonbox->radio) != button) {
	FlipColors(*(buttonbox->radio));
	FlipColors(button);
	*(buttonbox->radio) = button;
    }
    LastButtonPressed = button;
    (*(button->func))(scrn);
}


/* Act as if the last button pressed was pressed again. */

void RedoLastButton()
{
    if (LastButtonPressed)
	DoButtonPress(LastButtonPressed);
}


/* Returns the screen in which the last button was pressed. */

Scrn LastButtonScreen()
{
    return LastButtonPressed->buttonbox->scrn;
}


#ifdef X10
/* Handle X10 bug where we lose exit window events on the last button. */

ClearButtonTracks()
{
    XEvent event;
    event.type = LeaveWindow;
    event.window = LastButtonPressed->window;
    XtDispatchEvent(DISPLAY &event);
}
#endif X10
