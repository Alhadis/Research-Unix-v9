/* $Header: Actions.c,v 1.1 87/09/11 07:57:01 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Actions.c	1.12	2/25/87";
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

/* Actions.c -- parse all X events into widget specific actions. */

#include <stdio.h>
#include "Xlib.h"
#include "Xutil.h"
#include "Intrinsic.h"
#include <strings.h>
#include "Atoms.h"

/* Private definitions. */

#define NUMBUTTONS 3
#define NUMKEYS 256
#define HASH(x)	(((unsigned long)(x)) % NUMKEYS)
#define LF 0x0a
#define BSLASH '\\'

/* X10 Compatibility sillyness stuff (that should go away ...) */

#define MetaMask	Mod1Mask

typedef enum UpDown {down, up};
 
typedef struct _EventSeqRec {
    unsigned long eventCode;
    unsigned long eventType;
    short modeBits, modeMask;
    XtActionTokenPtr token;
    struct _EventSeqRec *next;
} EventSeqRec, *EventSeqPtr;

typedef struct _TranslationRec {
    XContext	 context;
    EventSeqPtr  *eventTable;
    XtActionsPtr actionTable;
} TranslationRec;

typedef struct {
    char    	*event;
    XContext  	context;
    short 	mode;
    unsigned long eventType;
}EventKey, *EventKeys;

EventKey events[] = {
    {"Key", 		NULL, 0, 		KeyPress},
    {"Ctrl", 		NULL, ControlMask, 	KeyPress},
    {"Meta", 		NULL, MetaMask, 	KeyPress},
    {"Shift", 		NULL, ShiftMask, 	KeyPress},
    {"BtnDown", 	NULL, 0, 		ButtonPress},
    {"BtnUp", 		NULL, 0, 		ButtonRelease},
    {"PtrMoved", 	NULL, 0, 		MotionNotify},
    {"KeyPress", 	NULL, 0, 		KeyPress},
    {"KeyRelease", 	NULL, 0, 		KeyRelease},
    {"ButtonPress", 	NULL, 0, 		ButtonPress},
    {"ButtonRelease", 	NULL, 0, 		ButtonRelease},
    {"MouseMoved", 	NULL, 0, 		MotionNotify},
    {"MotionNotify", 	NULL, 0, 		MotionNotify},
    {"EnterWindow", 	NULL, 0, 		EnterNotify},
    {"LeaveWindow", 	NULL, 0, 		LeaveNotify},
    {"FocusIn", 	NULL, 0, 		FocusIn},
    {"FocusOut", 	NULL, 0, 		FocusOut},
    {"KeymapNotify", 	NULL, 0, 		KeymapNotify},
    {"Expose", 		NULL, 0, 		Expose},
    {"GraphicsExpose", 	NULL, 0, 		GraphicsExpose},
    {"NoExpose", 	NULL, 0, 		NoExpose},
    {"VisibilityNotify",NULL, 0, 		VisibilityNotify},
    {"CreateNotify", 	NULL, 0, 		CreateNotify},
    {"DestroyNotify", 	NULL, 0, 		DestroyNotify},
    {"UnmapNotify", 	NULL, 0, 		UnmapNotify},
    {"MapNotify", 	NULL, 0, 		MapNotify},
    {"MapRequest", 	NULL, 0, 		MapRequest},
    {"ReparentNotify", 	NULL, 0, 		ReparentNotify},
    {"ConfigureNotify", NULL, 0, 		ConfigureNotify},
    {"ConfigureRequest",NULL, 0, 		ConfigureRequest},
    {"GravityNotify", 	NULL, 0, 		GravityNotify},
    {"ResizeRequest", 	NULL, 0, 		ResizeRequest},
    {"CirculateNotify",	NULL, 0, 		CirculateNotify},
    {"CirculateRequest",NULL, 0, 		CirculateRequest},
    {"PropertyNotify",	NULL, 0, 		PropertyNotify},
    {"SelectionClear",	NULL, 0, 		SelectionClear},
    {"SelectionRequest",NULL, 0, 		SelectionRequest},
    {"SelectionNotify",	NULL, 0, 		SelectionNotify},
    {"ColormapNotify",	NULL, 0, 		ColormapNotify},
    {"ClientMessage",	NULL, 0, 		ClientMessage},
    { NULL, NULL, NULL, NULL}
};

static Boolean initialized = FALSE;
static Boolean parseError;

void ActionsInitialize()
{
    int i;
    void CvtStringToEventBindings();

    if (initialized)
    	return;
    initialized = TRUE;

    for (i = 0; events[i].event != NULL; i++)
         events[i].context = XAtomToContext(events[i].event);
    XrmRegisterTypeConverter(XrmRString, XtREventBindings,
			     CvtStringToEventBindings);
}

static int StrToNum(s)
    char *s;
{
    int base = 10;
    register int val = 0;
    register int c;
    if (*s == '0') {
	s++;
	if (*s == 'x' || *s == 'X') {
	    base = 16;
	    s++;
	} else
	    base = 8;
    }
    while (*s)
        switch (c = *s++) {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	        val = (val * base) + (c - '0');
		break;
	    case '8':
	    case '9':
	    	if (base < 10)
			return -1;
	        val = (val * base) + (c - '0');
		break;
	    case 'a':
	    case 'b':
	    case 'c':
	    case 'd':
	    case 'e':
	    case 'f':
	        if (base != 16)
			return -1;
	        val = (val * base) + (c - 'a' + 10);
		break;
	    case 'A':
	    case 'B':
	    case 'C':
	    case 'D':
	    case 'E':
	    case 'F':
	        if (base != 16)
			return -1;
	        val = (val * base) + (c - 'A' + 10);
		break;
	    default:
	    	return -1;
	}
	return val;		
}

static Syntax(str)
    char *str;
{
    (void) fprintf(stderr,
     "Event Sequence description error (ignored): %s\n", str);
}

static EventSeqPtr *NewEventTable()
{
    EventSeqPtr *eventTable;
    int i;

    eventTable = (EventSeqPtr *) XtMalloc(sizeof(EventSeqPtr)* NUMKEYS);
    for (i = 0; i < NUMKEYS; i++)
	eventTable[i] = (EventSeqPtr) NULL;
    return eventTable;
}

static XtActionTokenPtr FindAction(list, modeBits, eventCode, eventType)
  EventSeqPtr list;
  short modeBits;
  register int eventCode, eventType;
{
    register EventSeqPtr ptr;

    for (ptr = list; ptr != NULL; ptr = ptr->next) {
        if ((ptr->eventCode == eventCode) &&
	    (ptr->eventType == eventType) &&
	     (ptr->modeBits == (modeBits & ptr->modeMask)))
	    return ptr->token;
    }
    return NULL;
}

static MakeKeyOrButtonEntry(list, modeBits, modeMask, eventCode, eventType, token)
  EventSeqPtr *list;
  short modeBits, modeMask;
  unsigned long eventCode, eventType;
  XtActionTokenPtr token;
{
  register EventSeqPtr ptr;
  
  ptr = (EventSeqPtr) XtMalloc(sizeof(EventSeqRec));
  ptr->eventCode = eventCode;
  ptr->eventType = eventType;
  ptr->modeBits = modeBits;
  ptr->modeMask = modeMask;
  ptr->token = token;
  ptr->next = *list;
  *list = ptr;
}

static int ButtonMaskFromString(str)
  char *str;
{
    switch (StrToNum(str)) {
	case 1:
	    return Button1Mask;
	case 2:
	    return Button2Mask;
	case 3:
	    return Button3Mask;
	case 4:
	    return Button4Mask;
	case 5:
	    return Button5Mask;
    }
    if (strcmp(str, "left") == 0) return Button1Mask;
    if (strcmp(str, "middle") == 0) return Button2Mask;
    if (strcmp(str, "right") == 0) return Button3Mask;

    return 0;
}

static int ButtonFromString(str)
  char *str;
{
    switch (ButtonMaskFromString(str)) {
	case Button1Mask:
	    return Button1;
	case Button2Mask:
	    return Button2;
	case Button3Mask:
	    return Button3;
	case Button4Mask:
	    return Button4;
	case Button5Mask:
	    return Button5;
    }

    parseError = TRUE;
    return 0;
}

/*
static StringToToken(table, str, token)
  XtActionsPtr table;
  char *str;
  int *token;
{
    register XtActionsPtr tab = table;
    while (tab->string) {
	if (strcmp(tab->string, str) == 0) {
	    *token = tab->value;
	    return TRUE;
	}
	tab++;
    }
    return FALSE;
}
*/

static Boolean ParseChar(s, c)
  char **s;
  char *c;
{
    Boolean result = FALSE;
    register char *ptr;

    ptr = *s;
    *c = *ptr++;
    if (*c == '\\') {
	result = TRUE;
	*c = *ptr++;
	switch (*c) {
	    case 't': *c = '\t';	break;
	    case 'n': *c = '\n';	break;
	    case 'r': *c = '\r';	break;
	    case 'e': *c = '\033';	break;
	}
	if (*c >= '0' && *c <= '9') {
	    *c = (*c - '0') * 64 + (ptr[0] - '0') * 8 + ptr[1] - '0';
	    ptr += 2;
	}
    }
    *s = ptr;
    return result;
}


static ParseEventSequenceBinding(str, tbl)
  char *str;
  EventSeqPtr *tbl;
{
    char    str2[500], modeStr[50], typeStr[50],  detailStr[50];
    char   *ptr, *ptr2, c;
    Boolean backSlash;
    int     modeBits, modeMask, i, code;
    Boolean notFlag, done;
    XtActionTokenPtr tok, lasttok, firsttok;
    unsigned long   eventType;
    XContext eventContext;

    ptr = str;
    modeStr[0] = 0;
    typeStr[0] = 0;
    detailStr[0] = 0;
    parseError = FALSE;
    ptr2 = modeStr;
    while (*ptr != ':') {	/* find end of lhs  and collect pieces */
	switch (*ptr) {
	    case 0: 
	    case LF: 
		Syntax(str);
		return;

	    case BSLASH: 
		ptr++;
		*ptr2++ = *ptr++;
		break;
	    case '<': 
		*ptr2 = 0;
		ptr2 = typeStr;
		ptr++;
		break;
	    case '>': 
		*ptr2 = 0;
		ptr2 = detailStr;
		ptr++;
		break;
	    default: 
		*ptr2++ = *ptr++;
	}
    }
    *ptr2 = 0;
    ptr++;			/* step over ':' */
 /* process mode flags */
    ptr2 = modeStr;
    notFlag = FALSE;
    code = 0;
    modeBits = 0;
    modeMask = 0;
    while (*(ptr2)) {
	switch (*ptr2) {
	    case '~': 
		notFlag = TRUE;
		break;
	    case 'c':
		modeMask |= ControlMask;
		if (notFlag)
		    modeBits &= ~ControlMask;
		else
		    modeBits |= ControlMask;
		notFlag = FALSE;
		break;
	    case 'm': 
		modeMask |= MetaMask;
		if (notFlag)
		    modeBits &= ~MetaMask;
		else
		    modeBits |= MetaMask;
		notFlag = FALSE;
		break;
	    case 's': 
		modeMask |= ShiftMask;
		if (notFlag)
		    modeBits &= ~ShiftMask;
		else
		    modeBits |= ShiftMask;
		notFlag = FALSE;
		break;
	    case 'l': 
		modeMask |= LockMask;
		if (notFlag)
		    modeBits &= ~LockMask;
		else
		    modeBits |= LockMask;
		notFlag = FALSE;
		break;
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
		str2[0] = *ptr2;
		str2[1] = '\0';
		i = ButtonMaskFromString(str2);
		modeMask |= i;
		if (notFlag)
		    modeBits &= ~i;
		else
		    modeBits |= i;
		break;

	    default: 
		Syntax (str);
		return;

	}
	ptr2++;
    }
    if (detailStr[0] >= '0' && detailStr[0] <= '9')
	code = StrToNum (detailStr);
 /* check for valid event type and set detail */
    eventType = 0;
    eventContext = XAtomToContext(typeStr);
    for (i = 0; events[i].event != NULL; i++) {
        if (events[i].context == eventContext) {
	    modeBits |= events[i].mode;
	    modeMask |= events[i].mode;
	    eventType = events[i].eventType;
	    break;
	}
    }
    switch (eventType) {
        case KeyPress:
	case KeyRelease:
	    if (detailStr[0] >= '0' && detailStr[0] <= '9')
                code = StrToNum (detailStr);
	    else code = (detailStr[0] >= 'A' && detailStr[0] <= 'Z') ?
				(detailStr[0] + 'a' - 'A') : detailStr[0];
	    break;
        case ButtonPress:
             code = ButtonFromString (detailStr);
             break;
        case ButtonRelease:
	     code = ButtonFromString (detailStr);
	     break;
        case MotionNotify:
	     code = 0;
             modeBits |= ButtonMaskFromString(detailStr);
	     modeMask |= ButtonMaskFromString(detailStr);
             break;
        case 0:
             Syntax(str);
	     return;
    }
    done = FALSE;
    lasttok = NULL;
    firsttok = NULL;
 /* now process rhs */
    while (*ptr) {
	tok = (XtActionTokenPtr) XtMalloc (sizeof (XtActionTokenRec));
	tok->next = NULL;
	while (*ptr == ' ' || *ptr == '\t')
	    ptr++;
	switch (*ptr) {
	    case '\n': 
	    case '#':
	    case 0: 
		done = TRUE;
		XtFree ((char *) tok);
		break;
	    case '\'': 
		ptr++;
		(void) ParseChar (&ptr, &c);
		if (*ptr != '\'') {
		    Syntax (str);
		    return;
		}
		tok->type = XttokenChar;
		tok->value.c = c;
		ptr++;
		break;
	    case '"': 
		ptr++;
		ptr2 = str2;
		while (1) {
		    backSlash = ParseChar (&ptr, ptr2);
		    if (!backSlash && *ptr2 == '"')
			break;
		    if (!backSlash && *ptr2 == '\n') {
			Syntax (str);
			return;
		    }
		    ptr2++;
		}
		*ptr2 = 0;
		tok->type = XttokenString;
		tok->value.str =
		 strcpy (XtMalloc ((unsigned) strlen (str2) + 1), str2);
		break;
	    default: 
		ptr2 = str2;
		do {
		    (void) ParseChar (&ptr, ptr2);
		    c = *ptr2++;
		} while (c != ' ' && c != '\t' && c != '\n' && c != 0);
		*--ptr2 = 0;
		tok->type = XttokenAction;
		tok->value.action = XtAtomToAction (str2);
	}
	if (!done && tok) {
	    if (lasttok)
		lasttok->next = tok;
	    else
		firsttok = tok;
	    lasttok = tok;
	}
    }
 /* set action based on eventType */
    if (parseError)
	Syntax(str);
    else
	MakeKeyOrButtonEntry (&tbl[HASH(code)], modeBits, modeMask,
			      code, eventType, firsttok);

}

/*ARGSUSED*/
void CvtStringToEventBindings(dpy, fromVal, toVal)
  Display	*dpy;
  XrmValue	fromVal;
  XrmValue	*toVal;
{
    FILE *fid;
    char str[1024];
    static EventSeqPtr *eventTable; /* ||| STATIC */
    extern char *getenv();

    (void) sprintf (str, "%s/%s", getenv ("HOME"), (char *) fromVal.addr);
    fid = fopen (str, "r");
    if (fid == NULL) {
        (void) strcpy (str, LIBDIR);
        (void) strcat (str, (char *) fromVal.addr);
	fid = fopen (str, "r");
	if (fid == NULL) {
	    (void) fprintf (stderr,
	     " Can't find Event Bindings file: %s, using defaults\n",
	     fromVal.addr);
	    return;
	}
    }
    eventTable = NewEventTable();
    while (!feof (fid)) {
	(void) fgets (str, 1020, fid);
	if (str[0] != '#')
	    ParseEventSequenceBinding(str, eventTable);
    }
    (void) fclose(fid);
    (*toVal).size = sizeof(caddr_t);
    (*toVal).addr = (caddr_t) &eventTable;
}

/* Public routines. */

XtEventsPtr XtParseEventBindings(stringTable)
  register char **stringTable;
{
    register int i = 0;
    register EventSeqPtr *eventTable;

    eventTable = NewEventTable();
    while (stringTable[i]) {
        ParseEventSequenceBinding(stringTable[i], eventTable);
	i++;
    }
    return (XtEventsPtr)eventTable;
}

caddr_t XtSetActionBindings(dpy, eventTable, actionTable, defaultValue)
    Display	*dpy;
    XtEventsPtr  eventTable;
    register XtActionsPtr actionTable;
    caddr_t 	 defaultValue;
{
    register XContext context;
    register TranslationPtr state;

    state = (TranslationPtr)XtMalloc(sizeof(TranslationRec));
    context = XUniqueContext();
    state->actionTable = actionTable;
    state->eventTable = (EventSeqPtr *) eventTable;
    while (actionTable->string) {
	(void) XSaveContext (
		dpy, (Window) XtAtomToAction (actionTable->string),
		context, (caddr_t) actionTable->value);
	actionTable++;
    }
    (void) XSaveContext (dpy, (Window) NULL, context, defaultValue);
    state->context = context;
    return (caddr_t) state;
}

XtActionTokenPtr XtTranslateEvent(event, state)
  register XEvent *event;
  TranslationPtr state;
{
    register XtActionTokenPtr result;
    static  XtActionTokenRec temp;
    static char str[1024];	/* !!! STATIC ||| */
    register int code;
    register int modeBits;
    int          n;
    
    result = NULL;
    code = 0;
    modeBits = 0;
    switch (event->type) {
	case KeyPress:
	case KeyRelease:
	    code = XLookupKeysym(&event->xkey, 0);
	    modeBits = event->xkey.state;
	    break;
	case ButtonPress:
	case ButtonRelease:
	    code = event->xbutton.button;
	    modeBits = event->xbutton.state;
	    break;
	case MotionNotify:
	    modeBits = event->xmotion.state;
	    break;
	case EnterNotify:
	case LeaveNotify:
	    modeBits = event->xcrossing.state;
	    break;
    }
    result = FindAction (state->eventTable[HASH(code)], modeBits, code, event->type);
    if (result == NULL &&
	   (event->type == KeyPress || event->type == KeyRelease)) {
	n = XLookupString((XKeyEvent *)event, str, sizeof(str),
			  (KeySym *)NULL, (XComposeStatus *)NULL);
	if (n == 0)
	    result = NULL;
	else {
	    str[n] = 0;
	    temp.value.str = str;
	    temp.type = XttokenString;
	    temp.next = NULL;
	    result = &temp;
	}
    }
    return result;
}


caddr_t XtInterpretAction(dpy, state, action)
  Display *dpy;
  TranslationPtr state;
  XtAction action;
{
    caddr_t result;
    if (XFindContext(
	dpy, (Window) action, state->context, (caddr_t *) &result))
	(void) XFindContext(dpy, (Window) NULL, state->context,
			     (caddr_t *) &result);
    return result;
}


XtAddAction(dpy, context, string, action)
  Display *dpy;
  XContext context;
  char *string;
  int action;
{
    (void) XSaveContext(dpy, (Window) XAtomToContext(string),
			 context, (caddr_t) action);
}

