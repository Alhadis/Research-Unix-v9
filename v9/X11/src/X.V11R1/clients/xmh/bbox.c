#ifndef lint
static char rcs_id[] = "$Header: bbox.c,v 1.12 87/09/11 08:19:05 toddb Exp $";
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

/* bbox.c -- management of buttons and buttonbox's. */

/* This module implements a simple interface to buttonboxes, allowing a client
   to create new buttonboxes and manage their contents.  It is a layer hiding
   the toolkit interfaces. */

#include "xmh.h"
#include "bboxint.h"


/* Free the storage for the given button. */

FreeButton(button)
Button button;
{
    XtFree(button->name);
    XtFree((char *) button);
}



/* Handle a buttonbox getting resized.  In particular, tell the VPane widget
   the new range of legal sizes for this buttonbox. */

#ifdef X11
static XtEventReturnCode HandleBBoxResize(event, buttonbox)
XEvent *event;
ButtonBox buttonbox;
{
    switch (event->type) {
      case ConfigureNotify:
	if (event->xconfigure.height != buttonbox->maxheight) {
	    buttonbox->maxheight = event->xconfigure.height;
	    XtVPanedSetMinMax(DISPLAY buttonbox->scrn->window,
			      buttonbox->outer,
			      buttonbox->fullsized ? buttonbox->maxheight : 5,
			      buttonbox->maxheight);
	}
	return XteventHandled;
      case DestroyNotify:
	XtFree((char *) buttonbox->button);
	XtFree((char *) buttonbox);
	return XteventHandled;
    }
    return XteventNotHandled;
}
#endif X11

#ifdef X10
static XtEventReturnCode HandleBBoxResize(event, buttonbox)
XExposeEvent *event;
ButtonBox buttonbox;
{
    switch (event->type) {
      case ResizeWindow:
	if (event->height != buttonbox->maxheight) {
	    buttonbox->maxheight = event->height;
	    XtVPanedSetMinMax(DISPLAY buttonbox->scrn->window,
			      buttonbox->outer,
			      buttonbox->fullsized ? buttonbox->maxheight : 5,
			      buttonbox->maxheight);
	}
	return XteventHandled;
      case DestroyWindow:
	XtFree((char *) buttonbox->button);
	XtFree((char *) buttonbox);
	return XteventHandled;
    }
    return XteventNotHandled;
}
#endif X10    


/* Create a new button box.  The window for it will be a child of the given
   scrn's window, and it will be added to the scrn's vpane. */

ButtonBox BBoxRadioCreate(scrn, position, name, radio)
  Scrn scrn;
  int position;			/* Position to put it in the vpane. */
  char *name;			/* Name of the buttonbox widget. */
  Button *radio;		/* Pointer to place to store which radio
				   button is active. */
{
    static Arg arglist[] = {
	{XtNname, (XtArgVal)NULL},
	{XtNwidth, NULL},
	{XtNinnerWidth, NULL},
	{XtNallowVert, (XtArgVal)TRUE},
    };
    static Arg arglist2[] = {
	{XtNwidth, NULL},
    };
    int width, height;
    ButtonBox buttonbox;

    arglist[0].value = (XtArgVal)name;
    GetWindowSize(scrn->window, &width, &height);
    arglist[1].value = arglist[2].value = arglist2[0].value = (XtArgVal) width;
    buttonbox = (ButtonBox) XtMalloc(sizeof(ButtonBoxRec));
    bzero((char *)buttonbox, sizeof(ButtonBoxRec));
    buttonbox->updatemode = TRUE;
    buttonbox->scrn = scrn;
    buttonbox->outer = XtScrolledWindowCreate(DISPLAY scrn->window, arglist,
					      XtNumber(arglist));
    buttonbox->inner =
	XtButtonBoxCreate(DISPLAY XtScrolledWindowGetFrame(DISPLAY
							   buttonbox->outer),
			  arglist2, XtNumber(arglist2));
    XtScrolledWindowSetChild(DISPLAY buttonbox->outer, buttonbox->inner);
    buttonbox->numbuttons = 0;
    buttonbox->button = (Button *) XtMalloc(1);
    XtVPanedWindowAddPane(DISPLAY scrn->window, buttonbox->outer, position, 5,
			  5, FALSE);
    buttonbox->maxheight = 5;
    buttonbox->radio = radio;
    if (radio) *radio = NULL;
    XtSetEventHandler(DISPLAY buttonbox->inner, HandleBBoxResize,
#ifdef X11
		      StructureNotifyMask,
#endif
#ifdef X10
		      ExposeWindow,
#endif
		      (caddr_t) buttonbox);
    return buttonbox;
}



/* Create a new buttonbox which does not manage radio buttons. */

ButtonBox BBoxCreate(scrn, position, name)
  Scrn scrn;
  int position;
  char *name;
{
    return BBoxRadioCreate(scrn, position, name, (Button *)NULL);
}



/* Set the current button in a radio buttonbox. */

void BBoxSetRadio(buttonbox, button)
ButtonBox buttonbox;
Button button;
{
    if (buttonbox->radio && *(buttonbox->radio) != button) {
	if (*(buttonbox->radio)) FlipColors(*(buttonbox->radio));
	FlipColors(button);
	*(buttonbox->radio) = button;
    }
}



/* Some buttons have been added to the buttonbox structure; go and actually
   add the button widgets to the buttonbox widget.  (It is much more
   efficient to add several buttons at once rather than one at a time.) */

static ProcessAddedButtons(buttonbox)
ButtonBox buttonbox;
{
    int i, position, index;
    ArgList arglist, ptr;
    Button button;
    if (buttonbox->updatemode == FALSE) {
	buttonbox->needsadding = TRUE;
	return;
    }
    position = 0;
    ptr = arglist = (ArgList)
	XtMalloc((unsigned)sizeof(Arg) * (buttonbox->numbuttons + 1));
    for (i=0 ; i<buttonbox->numbuttons ; i++) {
	button = buttonbox->button[i];
	if (button->needsadding) {
	    if (ptr == arglist) {
		ptr->name = XtNindex;
		index = position;
		ptr->value = (XtArgVal)index;
		ptr++;
	    }
	    ptr->name = XtNwindow;
	    ptr->value = (XtArgVal)(button->window);
	    ptr++;
	    button->needsadding = FALSE;
	} else if (ptr != arglist) {
	    (void)XtButtonBoxAddButton(DISPLAY buttonbox->inner,
				       arglist, ptr-arglist);
	    ptr = arglist;
	}
	position++;
    }
    if (ptr != arglist) {
	(void)XtButtonBoxAddButton(DISPLAY buttonbox->inner,
				   arglist, ptr-arglist);
    }
    XtFree((char *) arglist);
    buttonbox->needsadding = FALSE;
}



/* Create a new button, and add it to a buttonbox. */

void BBoxAddButton(buttonbox, name, func, position, enabled)
  ButtonBox buttonbox;
  char *name;			/* Name of button. */
  void (*func)();		/* Func to call when button pressed. */
  int position;			/* Position to put button in box. */
  int enabled;			/* Whether button is initially enabled. */
{
    extern void DoButtonPress();
    Button button;
    int i;
    static Arg arglist[] = {
	{XtNname, NULL},
	{XtNfunction, (XtArgVal)DoButtonPress},
	{XtNparameter, NULL},
    };
    if (position > buttonbox->numbuttons) position = buttonbox->numbuttons;
    buttonbox->numbuttons++;
    buttonbox->button = (Button *)
	XtRealloc((char *) buttonbox->button,
		  (unsigned) buttonbox->numbuttons * sizeof(Button));
    for (i=buttonbox->numbuttons-1 ; i>position ; i--)
	buttonbox->button[i] = buttonbox->button[i-1];
    button = buttonbox->button[position] =
	(Button) XtMalloc(sizeof(ButtonRec));
    bzero((char *) button, sizeof(ButtonRec));
    arglist[0].value = (XtArgVal)name;
    arglist[2].value = (XtArgVal)button;
    button->buttonbox = buttonbox;
    button->name = MallocACopy(name);
    button->window = XtCommandCreate(DISPLAY buttonbox->inner, arglist,
				     XtNumber(arglist));
    button->func = func;
    button->needsadding = TRUE;
    button->enabled = TRUE;
    if (!enabled) BBoxDisable(button);
    ProcessAddedButtons(buttonbox);
    if (buttonbox->radio && *(buttonbox->radio) == NULL) {
	*(buttonbox->radio) = button;
	FlipColors(button);
    }
}



/* Remove the given button from its buttonbox.  The button widget is
   destroyed.  If it was the current button in a radio buttonbox, then the
   current button becomes the first button in the box. */

void BBoxDeleteButton(button)
Button button;
{
    ButtonBox buttonbox = button->buttonbox;
    int i, found, reradio;
    static Arg arglist[] = {
	{XtNwindow, (XtArgVal)NULL}
    };
    found = FALSE;
    for (i=0 ; i<buttonbox->numbuttons; i++) {
	if (found) buttonbox->button[i-1] = buttonbox->button[i];
	else if (buttonbox->button[i] == button) {
	    found = TRUE;
	    arglist[0].value = (XtArgVal) button->window;
	    (void)XtButtonBoxDeleteButton(DISPLAY buttonbox->inner,
					  arglist, XtNumber(arglist));
	    QXDestroyWindow(theDisplay, button->window);
	    (void) XtSendDestroyNotify(DISPLAY button->window);
	    reradio = (buttonbox->radio && *(buttonbox->radio) == button);
	    FreeButton(button);
	}
    }
    if (found) {
	buttonbox->numbuttons--;
	if (reradio) {
	    *(buttonbox->radio) = NULL;
	    if (buttonbox->numbuttons)
		BBoxSetRadio(buttonbox, buttonbox->button[0]);
	}
    }
}
	    


/* Enable or disable the given command button widget. */

static void SendEnableMsg(window, value)
Window window;
int value;			/* TRUE for enable, FALSE for disable. */
{
    static Arg arglist[] = {XtNsensitive, NULL};
    arglist[0].value = (XtArgVal) value;
    XtCommandSetValues(DISPLAY window, arglist, XtNumber(arglist));
}



/* Enable the given button (if it's not already). */

void BBoxEnable(button)
Button button;
{
    if (!button->enabled) {
	button->enabled = TRUE;
	SendEnableMsg(button->window, TRUE);
    }
}



/* Disable the given button (if it's not already). */

void BBoxDisable(button)
Button button;
{
    if (button->enabled) {
	button->enabled = FALSE;
	SendEnableMsg(button->window, FALSE);
    }
}


/* Given a buttonbox and a button name, find the button in the box with that
   short name. */

Button BBoxFindButtonNamed(buttonbox, name)
ButtonBox buttonbox;
char *name;
{
    int i;
    for (i=0 ; i<buttonbox->numbuttons; i++)
	if (strcmp(name, buttonbox->button[i]->name) == 0)
	    return buttonbox->button[i];
    return NULL;
}



/* Return the nth button in the given buttonbox. */

Button BBoxButtonNumber(buttonbox, n)
ButtonBox buttonbox;
int n;
{
    return buttonbox->button[n];
}



/* Return how many buttons are in a buttonbox. */

int BBoxNumButtons(buttonbox)
ButtonBox buttonbox;
{
    return buttonbox->numbuttons;
}


/* Given a button, return its name. */

char *BBoxNameOfButton(button)
Button button;
{
    return button->name;
}



/* The client calls this routine before doing massive updates to a buttonbox.
   It then must call BBoxStartUpdate when it's finished.  This allows us to
   optimize things.  Right now, the only optimization performed is to batch
   together requests to add buttons to a buttonbox. */

void BBoxStopUpdate(buttonbox)
ButtonBox buttonbox;
{
    buttonbox->updatemode = FALSE;
}



/* The client has finished its massive updates; go and handle any batched
   requests. */

void BBoxStartUpdate(buttonbox)
ButtonBox buttonbox;
{
    buttonbox->updatemode = TRUE;
    if (buttonbox->needsadding) ProcessAddedButtons(buttonbox);
}



/* Set things to always display the entire contents of this buttonbox.  In
   otherwords, tells the VPane to not let the user make this pane any smaller
   than the buttonbox. */

void BBoxForceFullSize(buttonbox)
ButtonBox buttonbox;
{
#ifdef X10	/* X10 toolkit doesn't have magic configurenotify events */
    WindowInfo info;
    XQueryWindow(buttonbox->inner, &info);
    buttonbox->maxheight = info.height;
#endif
    XtVPanedSetMinMax(DISPLAY buttonbox->scrn->window, buttonbox->outer, 
		      buttonbox->maxheight, buttonbox->maxheight);
    buttonbox->fullsized = TRUE;
}


/* Set things to allow the user to show only part of a buttonbox.  This is the
   default action. */

void BBoxAllowAnySize(buttonbox)
ButtonBox buttonbox;
{
#ifdef X10
    BBoxForceFullSize(buttonbox); /* VERY hackish; works only because of the
				     limited way I use BBoxForceFullSize.%%% */
#endif
    XtVPanedSetMinMax(DISPLAY buttonbox->scrn->window, buttonbox->outer, 
		      5, buttonbox->maxheight);
    buttonbox->fullsized = FALSE;
}



/* Destroy the given buttonbox. */

void BBoxDeleteBox(buttonbox)
ButtonBox buttonbox;
{
    if (buttonbox->radio) *(buttonbox->radio) = NULL;
    QXDestroyWindow(theDisplay, buttonbox->outer);
    (void) XtSendDestroyNotify(DISPLAY buttonbox->outer);
}



/* Change the borderwidth of the given button. */

void BBoxChangeBorderWidth(button, borderWidth)
Button button;
unsigned int borderWidth;
{
#ifdef X11
    XSetWindowBorderWidth(DISPLAY button->window, borderWidth);
#endif
}
