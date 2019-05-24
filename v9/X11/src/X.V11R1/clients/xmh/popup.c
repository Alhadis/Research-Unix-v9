/* $Header: popup.c,v 1.1 87/09/11 08:18:25 toddb Exp $ */
/* popup.c -- Handle pop-up windows. */

#include "xmh.h"

/*
static Window confirmwindow = NULL;
static char *confirmstring;

static Window confirmparent;
static count = 0;

static Window promptwindow;
static int (*promptfunc)();
static char promptstring[210];

extern TellPrompt();
extern DestroyPromptWindow();

ArgList arglist, promptarglist;
*/

/*
InitPopup()
{
}
*/

static Scrn lastscrn = NULL;
static char laststr[500];

static Window confirmwindow = NULL;
static int buttoncount = 0;
static Window promptwindow = NULL;
static void (*promptfunction)();

void CenterWindow(parent, child)
Window parent, child;
{
    int x, y, parentwidth, parentheight, childwidth, childheight;
    GetWindowSize(parent, &parentwidth, &parentheight);
    GetWindowSize(child, &childwidth, &childheight);
    x = (parentwidth - childwidth) / 2;
    y = (parentheight - childheight) / 2;
    if (x < 0) x = 0;
    QXMoveWindow(theDisplay, child, x, y);
}


int Confirm(scrn, str)
Scrn scrn;
char *str;
{
    static Arg arglist[] = {
	{XtNname, (XtArgVal)"confirm"}
    };
    extern void RedoLastButton();
    if (lastscrn == scrn && strcmp(str, laststr) == 0) {
	DestroyConfirmWindow();
	return TRUE;
    }
    DestroyConfirmWindow();
    lastscrn = scrn;
    scrn = LastButtonScreen();
    (void) strcpy(laststr, str);
    confirmwindow = XtDialogCreate(DISPLAY scrn->window, str, (char *)NULL,
				   arglist, XtNumber(arglist));
    XtDialogAddButton(DISPLAY confirmwindow,
		      "yes", RedoLastButton, (caddr_t)NULL);
    XtDialogAddButton(DISPLAY confirmwindow,
		      "no", DestroyConfirmWindow, (caddr_t)NULL);
    CenterWindow(scrn->window, confirmwindow);
    QXMapWindow(theDisplay, confirmwindow);
    buttoncount = 0;
    return FALSE;
}


DestroyConfirmWindow()
{
    lastscrn = NULL;
    *laststr = 0;
    if (confirmwindow) {
	QXDestroyWindow(theDisplay, confirmwindow);
	(void) XtSendDestroyNotify(DISPLAY confirmwindow);
	confirmwindow = NULL;
    }
}


HandleConfirmEvent(event)
XEvent *event;
{
    if (confirmwindow &&
	    (event->type == ButtonRelease || event->type == KeyPress)) {
	if (++buttoncount > 1)
	    DestroyConfirmWindow();
    }
}


DestroyPromptWindow()
{
    if (promptwindow) {
	(void) XtSendDestroyNotify(DISPLAY promptwindow);
	QXDestroyWindow(theDisplay, promptwindow);
	promptwindow = NULL;
    }
}


TellPrompt()
{
    (*promptfunction)(XtDialogGetValueString(DISPLAY promptwindow));
    DestroyPromptWindow();
}

MakePrompt(scrn, prompt, func)
Scrn scrn;
char *prompt;
void (*func)();
{
    static Arg arglist[] = {
	{XtNname, (XtArgVal)"prompt"}
    };
    DestroyPromptWindow();
    promptwindow = XtDialogCreate(DISPLAY scrn->window, prompt, "",
				  arglist, XtNumber(arglist));
    XtDialogAddButton(DISPLAY promptwindow,
		      "goAhead", TellPrompt, (caddr_t)NULL);
    XtDialogAddButton(DISPLAY promptwindow,
		      "cancel", DestroyPromptWindow,
		      (caddr_t)NULL);
    CenterWindow(scrn->window, promptwindow);
    QXMapWindow(theDisplay, promptwindow);
    promptfunction = func;
}
