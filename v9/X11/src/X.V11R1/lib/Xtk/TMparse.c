#ifndef lint
static char rcsid[] = "$Header: TMparse.c,v 1.24 87/09/11 21:24:33 haynes Rel $";
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
/* TMparse.c -- parse all X events into widget specific actions. */

#include <stdio.h>
#include "Xlib.h"
#include "Xutil.h"
#include "Intrinsic.h"
#include <strings.h>
#include "Atoms.h"
#include "TM.h"
#include "TMprivate.h"

/* Private definitions. */
#define LF 0x0a
#define BSLASH '\\'

#define AtomToAction(atom)	((XtAction)StringToQuark(atom))

typedef int		EventType;
typedef unsigned int	XtEventType;
typedef unsigned int	EventCode;
typedef unsigned int	Value;

typedef void (*ActionProc)();

typedef struct _EventKey {
    char    	*event;
    XrmQuark	signature;
    EventMask	mask;
    EventType	eventType;
    int		detailType;
    caddr_t	detail;
}EventKey, *EventKeys;

typedef struct {
    char	*name;
    XrmQuark	signature;
    Value	 value;
} NameValueRec, *NameValueTable;

typedef NameValueRec CompiledAction;
typedef NameValueTable CompiledActionTable;

NameValueRec modifiers[] = {
    {"Shift",	0,	ShiftMask},
    {"Lock",	0,	LockMask},
    {"Ctrl",	0,	ControlMask},
    {"Mod1",	0,	Mod1Mask},
    {"Mod2",	0,	Mod2Mask},
    {"Mod3",	0,	Mod3Mask},
    {"Mod4",	0,	Mod4Mask},
    {"Mod5",	0,	Mod5Mask},
    {"Meta",	0,	Mod1Mask},

    {"Button1",	0,	Button1Mask},
    {"Button2",	0,	Button2Mask},
    {"Button3",	0,	Button3Mask},
    {"Button4",	0,	Button4Mask},
    {"Button5",	0,	Button5Mask},

    {"Any",	0,	AnyModifier},

    {NULL, NULL, NULL},
};

NameValueRec buttonNames[] = {
    {"Button1",	0,	Button1},
    {"Button2", 0,	Button2},
    {"Button3", 0,	Button3},
    {"Button4", 0,	Button4},
    {"Button5", 0,	Button5},
    {NULL, NULL, NULL},
};

NameValueRec notifyModes[] = {
    {"Normal", 0,	NotifyNormal},
    {"Grab", 0,	NotifyGrab},
    {"Ungrab", 0,	NotifyUngrab},
    {"WhileGrabbed", 0,	NotifyWhileGrabbed},
    {NULL, NULL, NULL},
};

NameValueRec notifyDetail[] = {
    {"Ancestor", 0,	NotifyAncestor},
    {"Virtual", 0,	NotifyVirtual},
    {"Inferior", 0,	NotifyInferior},
    {"Nonlinear", 0,	NotifyNonlinear},
    {"NonlinearVirtual", 0,	NotifyNonlinearVirtual},
    {"Pointer", 0,	NotifyPointer},
    {"PointerRoot", 0,	NotifyPointerRoot},
    {"DetailNone", 0,	NotifyDetailNone},
    {NULL, NULL, NULL},
};

NameValueRec visibilityNotify[] = {
    {"Unobscured", 0,	VisibilityUnobscured},
    {"PartiallyObscured", 0,	VisibilityPartiallyObscured},
    {"FullyObscured", 0,	VisibilityFullyObscured},
    {NULL, NULL, NULL},
};

NameValueRec circulation[] = {
    {"OnTop", 0,	PlaceOnTop},
    {"OnBottom", 0,	PlaceOnBottom},
    {NULL, NULL, NULL},
};

NameValueRec propertyChanged[] = {
    {"NewValue", 0,	PropertyNewValue},
    {"Delete", 0,	PropertyDelete},
    {NULL, NULL, NULL},
};

#define NEM NoEventMask
#define KPM KeyPressMask
#define KRM KeyReleaseMask
#define BPM ButtonPressMask
#define BRM ButtonReleaseMask
#define EWM EnterWindowMask
#define LWM LeaveWindowMask
#define PMM PointerMotionMask
#define PHM PointerMotionHintMask
#define B1M Button1MotionMask
#define B2M Button2MotionMask
#define B3M Button3MotionMask
#define B4M Button4MotionMask
#define B5M Button5MotionMask
#define BMM ButtonMotionMask
#define KSM KeymapStateMask
#define EXM ExposureMask
#define VCM VisibilityChangeMask
#define STM StructureNotifyMask
#define RRM ResizeRedirectMask
#define SSM SubstructureNotifyMask
#define SRM SubstructureRedirectMask
#define FCM FocusChangeMask
#define PCM PropertyChangeMask
#define CCM ColormapChangeMask
#define OGM OwnerGrabButtonMask

#define DetailNone 0
#define DetailTable 1
#define DetailKeySym 2
#define DetailImmed 3

EventKey events[] = {
{"KeyPress",	    NULL, KPM, KeyPress,	DetailKeySym,	NULL},
{"KeyRelease",	    NULL, KRM, KeyRelease,	DetailKeySym,	NULL},
{"ButtonPress",     NULL, BPM, ButtonPress, DetailTable,(caddr_t)buttonNames},
{"ButtonRelease",   NULL, BRM, ButtonRelease,DetailTable,(caddr_t)buttonNames},
{"MotionNotify",    NULL, PMM, MotionNotify,	DetailNone,	NULL},
{"EnterNotify",     NULL, EWM, EnterNotify, DetailTable,(caddr_t)notifyModes},
{"LeaveNotify",     NULL, LWM, LeaveNotify, DetailTable,(caddr_t)notifyModes},
{"FocusIn",	    NULL, FCM, FocusIn,	    DetailTable,(caddr_t)notifyModes},
{"FocusOut",	    NULL, FCM, FocusOut,    DetailTable,(caddr_t)notifyModes},
{"KeymapNotify",    NULL, KSM, KeymapNotify,	DetailNone,	NULL},
{"Expose", 	    NULL, EXM, Expose,		DetailNone,	NULL},
{"GraphicsExpose",  NULL, EXM, GraphicsExpose,	DetailNone,	NULL},
{"NoExpose",	    NULL, EXM, NoExpose,	DetailNone,	NULL},
{"VisibilityNotify",NULL, VCM, VisibilityNotify,DetailNone,	NULL},
{"CreateNotify",    NULL, STM, CreateNotify,	DetailNone,	NULL},
{"DestroyNotify",   NULL, STM, DestroyNotify,	DetailNone,	NULL},
{"UnmapNotify",     NULL, STM, UnmapNotify,	DetailNone,	NULL},
{"MapNotify",	    NULL, STM, MapNotify,	DetailNone,	NULL},
{"MapRequest",	    NULL, SRM, MapRequest,	DetailNone,	NULL},
{"ReparentNotify",  NULL, STM, ReparentNotify,	DetailNone,	NULL},
{"ConfigureNotify", NULL, STM, ConfigureNotify,	DetailNone,	NULL},
{"ConfigureRequest",NULL, SRM, ConfigureRequest,DetailNone,	NULL},
{"GravityNotify",   NULL, STM, GravityNotify,	DetailNone,	NULL},
{"ResizeRequest",   NULL, RRM, ResizeRequest,	DetailNone,	NULL},
{"CirculateNotify", NULL, STM, CirculateNotify,	DetailNone,	NULL},
{"CirculateRequest",NULL, SRM, CirculateRequest,DetailNone,	NULL},
{"PropertyNotify",  NULL, PCM, PropertyNotify,	DetailNone,	NULL},
{"SelectionClear",  NULL, SRM, SelectionClear,	DetailNone,	NULL},
{"SelectionRequest",NULL, SRM, SelectionRequest,DetailNone,	NULL},
{"SelectionNotify", NULL, SRM, SelectionNotify,	DetailNone,	NULL},
{"ColormapNotify",  NULL, CCM, ColormapNotify,	DetailNone,	NULL},
{"ClientMessage",   NULL, 0,   ClientMessage,	DetailNone,	NULL},
{"MappingNotify",   NULL, 0,   0/*mapping*/,	DetailNone,	NULL},

{"Key", 	    NULL, KPM, KeyPress,	DetailKeySym,	NULL},
{"BtnDown",	    NULL, BPM, ButtonPress, DetailTable,(caddr_t)buttonNames},
{"BtnUp", 	    NULL, BRM, ButtonRelease,DetailTable,(caddr_t)buttonNames},
{"Btn1Down",	    NULL, BPM, ButtonPress,	DetailImmed,(caddr_t)Button1},
{"Btn1Up", 	    NULL, BRM, ButtonRelease,	DetailImmed,(caddr_t)Button1},
{"Btn2Down", 	    NULL, BPM, ButtonPress,	DetailImmed,(caddr_t)Button2},
{"Btn2Up", 	    NULL, BRM, ButtonRelease,	DetailImmed,(caddr_t)Button2},
{"Btn3Down", 	    NULL, BPM, ButtonPress,	DetailImmed,(caddr_t)Button3},
{"Btn3Up", 	    NULL, BRM, ButtonRelease,	DetailImmed,(caddr_t)Button3},
{"Btn4Down", 	    NULL, BPM, ButtonPress,	DetailImmed,(caddr_t)Button4},
{"Btn4Up", 	    NULL, BRM, ButtonRelease,	DetailImmed,(caddr_t)Button4},
{"Btn5Down", 	    NULL, BPM, ButtonPress,	DetailImmed,(caddr_t)Button5},
{"Btn5Up", 	    NULL, BRM, ButtonRelease,	DetailImmed,(caddr_t)Button5},
{"PtrMoved", 	    NULL, PMM, MotionNotify,	DetailNone,	NULL},
{"MouseMoved", 	    NULL, PMM, MotionNotify,	DetailNone,	NULL},
{"EnterWindow",     NULL, EWM, EnterNotify, DetailTable,(caddr_t)notifyModes},
{"LeaveWindow",     NULL, LWM, LeaveNotify, DetailTable,(caddr_t)notifyModes},

{ NULL, NULL, NULL, NULL, NULL, NULL}};

static Boolean initialized = FALSE;

static void FreeEventSeq(event)
  EventSeqPtr event;
{
  if (event->next != NULL)
	FreeEventSeq(event->next);
  XtFree((char *)event->str);
  XtFree((char *)event);
}

static void CompileNameValueTable(table)
    NameValueTable table;
{
    int i;

    for (i=0; table[i].name; i++)
        table[i].signature = StringToQuark(table[i].name);
}

static void Compile_XtEventTable(table)
    EventKeys	table;
{
    int i;

    for (i=0; table[i].event; i++)
        table[i].signature = StringToQuark(table[i].event);
}

static CompiledActionTable CompileActionTable(actions, count)
    struct _XtActionsRec *actions;
    Cardinal count;
{
    int i;
    CompiledActionTable compiledActionTable;

    compiledActionTable = (CompiledActionTable) XtCalloc(
	count+1, (unsigned) sizeof(CompiledAction));

    for (i=0; i<count; i++) {
	compiledActionTable[i].name = actions[i].string;
	compiledActionTable[i].signature = AtomToAction(actions[i].string);
	compiledActionTable[i].value = (Value) actions[i].value;
    }

    compiledActionTable[count].name = NULL;
    compiledActionTable[count].signature = NULL;
    compiledActionTable[count].value = NULL;

    return compiledActionTable;
}

static void FreeCompiledActionTable(compiledActionTable)
    CompiledActionTable compiledActionTable;
{
    XtFree((char *)compiledActionTable);
}

static Syntax(str)
    char *str;
{
    (void) fprintf(stderr,
     "Translation table syntax error: %s\n", str);
}



static XtEventType LookupXtEventType(eventStr)
  char *eventStr;

{
    int i;
    XrmQuark	signature;

    signature = StringToQuark(eventStr);
    for (i = 0; events[i].event != NULL; i++)
        if (events[i].signature == signature) return i;

    Syntax("Unknown event type.");
    return i;
}

#ifdef notdef
/* ||| */
{
    /*** Parse the repetitions, for double click... ***/
    if (strcmp(repsStr, "") != NULL) {
	EventSeqPtr tempEvent = curEvent;
	int reps;
        if (repsStr[0] >= '0' && repsStr[0] <= '9')
            reps = StrToNum (repsStr);
        else
	    reps = 1;
	for (i=1; i<reps; i++) {
	    curEvent->next = (EventSeqPtr) XtMalloc((unsigned)sizeof(EventSeqRec));
	    curEvent = curEvent->next;
	    curEvent->str = NULL;
	    curEvent->next = NULL;
	    curEvent->eventCode = tempEvent->eventCode;
	    curEvent->eventType = tempEvent->eventType;
	    curEvent->modifiersMask = tempEvent->modifiersMask;
	}
    }
}
#endif

/***********************************************************************
 * LookupTableSym
 * Given a table and string, it fills in the value if found and returns
 * status
 ***********************************************************************/

static Boolean LookupTableSym(table, name, valueP)
    NameValueTable	table;
    char *name;
    Value *valueP;
{
/* !!! should implement via hash or something else faster than linear search */

    int i;
    XrmQuark	signature = StringToQuark(name);

    for (i=0;table[i].name != NULL;i++)
	if (table[i].signature == signature) {
	    *valueP = table[i].value;
	    return TRUE;
	}

    return FALSE;
}

/***********************************************************************
 * InterpretAction
 * Given an action, it returns a pointer to the appropriate procedure.
 ***********************************************************************/

static ActionProc InterpretAction(compiledActionTable, action)
    CompiledActionTable compiledActionTable;
    String action;
{
    Value actionProc;

    if (LookupTableSym(compiledActionTable, action, &actionProc))
	return (ActionProc) actionProc;

    return NULL;
}

static char * ScanAlphanumeric(str)
    char *str;
{
    while (
        ('A' <= *str && *str <= 'Z') || ('a' <= *str && *str <= 'z')
	|| ('0' <= *str && *str <= '9')) str++;
    return str;
}

static char * ScanIdent(str)
    char *str;
{
    str = ScanAlphanumeric(str);
    while (
	   ('A' <= *str && *str <= 'Z')
	|| ('a' <= *str && *str <= 'z')
	|| ('0' <= *str && *str <= '9')
	|| (*str == '-')
	|| (*str == '_')
	|| (*str == '$')
	) str++;
    return str;
}

static char * ScanWhitespace(str)
    char *str;
{
    while (*str == ' ' || *str == '\t') str++;
    return str;
}

static char * ParseModifiers(str, modifierMaskP, modifierP)
    char *str;
    ModifierMask *modifierMaskP;
    ModifierMask *modifierP;
{
    char *start;
    char modStr[100];
    Boolean notFlag;
    Value maskBit;

    while (*str != '<') {
	str = ScanWhitespace(str);
	if (*str == '~') { notFlag = TRUE; str++; } else notFlag = FALSE;
	start = str;
        str = ScanAlphanumeric(str);
	if (start == str) {
	    Syntax("Modifier or '<' expected.");
	    return str;
	}
	(void) strncpy(modStr, start, str-start);
	modStr[str-start] = '\0';
	maskBit = 0;
	if (!LookupTableSym(modifiers, modStr, &maskBit))
	    Syntax("Unknown modifier name.");
	*modifierMaskP |= maskBit;
	if (notFlag) *modifierP &= ~maskBit; else *modifierP |= maskBit;
    }
    return str;
}

static char * ParseXtEventType(str, eventTypeP)
    char *str;
    XtEventType *eventTypeP;
{
    char *start = str;
    char eventTypeStr[100];

    str = ScanAlphanumeric(str);
    (void) strncpy(eventTypeStr, start, str-start);
    eventTypeStr[str-start] = '\0';
    *eventTypeP = LookupXtEventType(eventTypeStr);

    return str;
}

static unsigned int StrToHex(str)
    char *str;
{
    char c;
    int	val = 0;

    while (c = *str) {
	if ('0' <= c && c <= '9') val = val*16+c-'0';
	else if ('a' <= c && c <= 'z') val = val*16+c-'a'+10;
	else if ('A' <= c && c <= 'Z') val = val*16+c-'A'+10;
	else return -1;
	str++;
    }

    return val;
}

static unsigned int StrToOct(str)
    char *str;
{
    char c;
    int	val = 0;

    while (c = *str) {
        if ('0' <= c && c <= '7') val = val*8+c-'0'; else return -1;
	str++;
    }

    return val;
}

static unsigned int StrToNum(str)
    char *str;
{
    char c;
    int	val = 0;

    if (*str == '0') {
	str++;
	if (*str == 'x' || *str == 'X') return StrToHex(++str);
	else return StrToOct(str);
    }

    while (c = *str) {
	if ('0' <= c && c <= '9') val = val*10+c-'0';
	else return -1;
	str++;
    }

    return val;
}

static KeySym XStringToKeySym(str)
    char *str;
{

/* ||| replace this with real one when xlib has it... */

    if (str == NULL) return (KeySym) 0;
    if ('0' <= *str && *str <= '9') return (KeySym) StrToNum(str);
    if ('A' <= *str && *str <= 'Z') return (KeySym) *str+'a'-'A';
    return (KeySym) *str;
}

static char * ParseKeySym(str, eventCodeMaskP, eventCodeP)
    char *str;
    EventCode *eventCodeMaskP;
    EventCode *eventCodeP;
{
    char keySymName[100], *start;

    str = ScanWhitespace(str);

    if (*str == '\\') {
	str++;
	keySymName[0] = *str;
	str++;
	keySymName[1] = '\0';
	*eventCodeP = XStringToKeySym(keySymName);
	*eventCodeMaskP = ~0L;
    } else if (*str == ',' || *str == ':') {
	/* no detail */
	*eventCodeP = 0L;
        *eventCodeMaskP = 0L;
    } else {
	start = str;
	while (*str != ',' && *str != ':') str++;
	(void) strncpy(keySymName, start, str-start);
	keySymName[str-start] = '\0';
	*eventCodeP = XStringToKeySym(keySymName);
	*eventCodeMaskP = ~0L;
    }

    return str;
}


static char * ParseTableSym(str, table, eventCodeMaskP, eventCodeP)
    char *str;
    NameValueTable table;
    EventCode *eventCodeMaskP;
    EventCode *eventCodeP;
{
    char *start = str;
    char tableSymName[100];

    *eventCodeP = 0L;
    str = ScanAlphanumeric(str);
    if (str == start) {*eventCodeMaskP = 0L; return str; }
    (void) strncpy(tableSymName, start, str-start);
    tableSymName[str-start] = '\0';
    if (! LookupTableSym(table, tableSymName, eventCodeP))
	Syntax("Unknown Detail Type.");
    *eventCodeMaskP = ~0L;

    return str;
}


static char * ParseDetail(str, eventType, eventCodeMaskP, eventCodeP)
    char *str;
    XtEventType eventType;
    EventCode	*eventCodeMaskP;
    EventCode	*eventCodeP;
{
    switch (events[eventType].detailType) {

	case DetailImmed:
	    *eventCodeMaskP = ~0L;
	    *eventCodeP = (EventCode) events[eventType].detail;
	    return str;

	case DetailKeySym:
	    str = ParseKeySym(str, eventCodeMaskP, eventCodeP);
	    return str;

	case DetailTable:
	    str = ParseTableSym(
		str, (NameValueTable)events[eventType].detail,
		eventCodeMaskP, eventCodeP);
	    return str;

	default:
	    *eventCodeMaskP = 0L;
	    *eventCodeP = 0L;
	    return str;
    }
}


static char * ParseEvent(str, eventP)
    char *str;
    EventSeqPtr	eventP;
{
    ModifierMask modifierMask = 0;
    ModifierMask modifiers = 0;
    XtEventType	eventType = 0;
    EventCode	eventCodeMask = 0L;
    EventCode	eventCode = 0L;

    str = ParseModifiers(str, &modifierMask, &modifiers);
    if (*str != '<') Syntax("Missing '<'"); else str++;
    str = ParseXtEventType(str, &eventType);
    if (*str != '>') Syntax("Missing '>'"); else str++;
    str = ParseDetail(str, eventType, &eventCodeMask, &eventCode);

    eventP->modifierMask = modifierMask;
    eventP->modifiers = modifiers;
    eventP->eventType = (EventType)eventType;
    eventP->eventCodeMask = eventCodeMask;
    eventP->eventCode = eventCode;

    return str;
}

static char * ParseQuotedStringEvent(str, eventP)
    char *str;
    EventSeqPtr eventP;
{
    int j;

    ModifierMask ctrlMask;
    ModifierMask metaMask;
    ModifierMask shiftMask;
    char	c;
    char	s[2];

    (void) LookupTableSym(modifiers, "Ctrl", (Value *) &ctrlMask);
    (void) LookupTableSym(modifiers, "Meta", (Value *) &metaMask);
    (void) LookupTableSym(modifiers, "Shift", (Value *) &shiftMask);

    eventP->modifierMask = ctrlMask | metaMask | shiftMask;

    for (j=0; j < 2; j++)
	if (*str=='^' && !(eventP->modifiers | ctrlMask)) {
	    str++;
	    eventP->modifiers |= ctrlMask;
	} else if (*str == '$' && !(eventP->modifiers | metaMask)) {
	    str++;
	    eventP->modifiers |= metaMask;
	} else if (*str == '\\') {
	    str++;
	    c = *str;
	    str++;
	    break;
	} else {
	    c = *str;
	    str++;
	    break;
	}
    eventP->eventType = (EventType) LookupXtEventType("Key");
    if ('A' <= c && c <= 'Z') {
	eventP->modifiers |=  shiftMask;
	c += 'a' - 'A';
    }
    s[0] = c;
    s[1] = '\0';
    eventP->eventCode = XStringToKeySym(s);

    return str;
}


/***********************************************************************
 * ParseEventSeq
 * Parses the left hand side of a translation table production
 * up to, and consuming the ":".
 * Takes a pointer to a char* (where to start parsing) and returns an
 * event seq (in a passed in variable), having updated the char *
 **********************************************************************/

static char *ParseEventSeq(str, eventSeqP)
    char *str;
    EventSeqPtr *eventSeqP;
{
    EventSeqPtr *nextEventP = eventSeqP;

    *eventSeqP = NULL;

    while (*str != ':') {
	EventSeqPtr	event;

	event = (EventSeqPtr) XtMalloc((unsigned)sizeof(EventSeqRec));
	event->str = NULL;
        event->modifierMask = 0;
        event->modifiers = 0;
        event->eventType = 0;
        event->eventCodeMask = 0L;
        event->eventCode = 0L;
        event->next = NULL;
        event->actions = NULL;

	if (*str == '"') {
	    str++;
	    while (*str != '"') {
		str = ParseQuotedStringEvent(str, event);
		*nextEventP = event;
		nextEventP = &event->next;
	    }
	    str++;
	} else {
	    str = ParseEvent(str, event);
	    *nextEventP = event;
	    nextEventP = &event->next;
	}
	str = ScanWhitespace(str);
	if (*str != ':')
	    if (*str != ',') {
		Syntax("',' expected.");
	    } else str++;
    }
    str++;

    return str;
}


static char * ParseActionProc(str, actionProcP, actionProcNameP)
    char *str;
    ActionProc *actionProcP;
    char **actionProcNameP;
{
    char *start = str;
    char procName[100];

    str = ScanIdent(str);
    (void) strncpy(procName, start, str-start);
    procName[str-start] = '\0';

/* ||| */
/*
    if (! LookupTableSym(procName, actions, actionProcP))
	Syntax("Unkown action proc.");
*/
    *actionProcP = NULL;
    *actionProcNameP = strncpy(
	XtMalloc((unsigned)(str-start+1)), procName, str-start+1);
    return str;
}


static char * ParseParamSeq(str, paramSeqP, paramNumP)
    char *str;
    char ***paramSeqP;
    unsigned long *paramNumP;
{
    /* ||| */
    *paramSeqP = NULL;
    *paramNumP = 0;

    return str;
}

static char * ParseAction(str, actionP)
    char *str;
    ActionPtr actionP;
{
    str = ParseActionProc(str, &actionP->proc, &actionP->token);
    if (*str == '(') {
	str++;
	str = ParseParamSeq(str, &actionP->param, &actionP->paramNum);
    } else { Syntax("Missing '('"); }
    if (*str == ')') str++; else Syntax("Missing ')'");

    return str;
}


static char *ParseActionSeq(str, actionsP)
    char *str;
    ActionPtr *actionsP;
{
    ActionPtr *nextActionP = actionsP;

    *actionsP = NULL;

    while (*str != '\0') {
	ActionPtr	action;

	action = (ActionPtr) XtMalloc((unsigned)sizeof(ActionRec));
        action->token = NULL;
        action->proc = NULL;
        action->param = NULL;
        action->paramNum = 0;
        action->next = NULL;

	str = ParseAction(str, action);
	str = ScanWhitespace(str);
	*nextActionP = action;
	nextActionP = &action->next;
    }

    return str;
}


/***********************************************************************
 * ParseTranslationTableProduction
 * Parses one line of event bindings.
 ***********************************************************************/

static void ParseTranslationTableProduction(w, compiledActionTable, str)
  Widget w;
  CompiledActionTable	compiledActionTable;
  char *str;
{
    EventSeqPtr	eventSeq = NULL;
    ActionPtr	actions = NULL;

    EventSeqPtr	esp;
    ActionPtr	ap;

    str = ParseEventSeq(str, &eventSeq);
    str = ScanWhitespace(str);
    str = ParseActionSeq(str, &actions);

    for (esp=eventSeq; esp!=NULL; esp=esp->next) {
	XtAddEventHandler(
	    w,
	    events[esp->eventType].mask,
	    (Boolean) (events[esp->eventType].mask == 0),
	    TranslateEvent, (Opaque) NULL);
#ifdef ndef
	/* double click needs to make sure that you have selected on both
	    button down and up. */
        if (events[esp->eventType].mask & ButtonPressMask)
	    XtAddEventHandler(
		w, ButtonReleaseMask, FALSE, TranslateEvent, NULL);
        if (events[esp->eventType].mask & ButtonReleaseMask)
	    XtAddEventHandler(
		w, ButtonPressMask, FALSE, TranslateEvent, NULL);
#endif
	if (esp->next == NULL) {
	    /* put the action procs in at the end */
	    esp->actions = actions;
	}
        /* change translation manager events into real x events */
	esp->eventType = events[esp->eventType].eventType;
	/* don't use esp->eventType as an index into event from here on... */
	w->core.translations->eventObjTbl = EventMapObjectCreate(
	    w->core.translations, esp);
    }

    /* run down the action list, binding action names to procs */
    for (ap=actions; ap!=NULL; ap=ap->next) {
        ap->proc = InterpretAction(compiledActionTable, ap->token);
    }

    /* add the events and actions to the state table */
    w->core.translations->eventObjTbl = EventMapObjectSet(
	w->core.translations, eventSeq);

    FreeEventSeq(eventSeq);
    
}

/*
 * Parses a user's or applications translation table
 */

static void ParseTranslationTable(w, compiledActionTable)
    Widget w;
    CompiledActionTable	compiledActionTable;
{
    char **translationTableSource = (char **)w->core.translations;
    int i;

    w->core.translations =
	(_XtTranslations) XtMalloc((unsigned) sizeof(TranslationData));
    w->core.translations->numEvents = 0;
    w->core.translations->eventTblSize = 0;
    w->core.translations->eventObjTbl = NULL;

    /* !!! need some way of setting this !!! */
    w->core.translations->clickTime = 50;

    i = 0;
    while (translationTableSource[i]) {
        ParseTranslationTableProduction(
	    w, compiledActionTable, translationTableSource[i]);
	i++;
    }

}

/*** public procedures ***/

void DefineTranslation(w)
  Widget w;
{
    /* this procedure assumes that there is a string table in the */
    /* core.translations field. It compiles it, combines it with */
    /* the action bindings and puts the resulting internal data */
    /* structure into the core.translations field. Note that this means */
    /* that if you call DefineTranslation twice, bad things will happen */

    CompiledActionTable	compiledActionTable;

    if (w->core.widget_class->core_class.actions == NULL) return;

    compiledActionTable =
	CompileActionTable(
	    w->core.widget_class->core_class.actions,
	    w->core.widget_class->core_class.num_actions);
    ParseTranslationTable(w, compiledActionTable);
    FreeCompiledActionTable(compiledActionTable);


    /* double click needs to make sure that you have selected on both
       button down and up. */
#ifdef ndef
    if (w->core.event_mask & ButtonPressMask || 
        w->core.event_mask & ButtonReleaseMask)
	   w->core.event_mask |= ButtonPressMask | ButtonReleaseMask;
#endif
}

void TranslateInitialize()
{
    if (initialized) return;

    initialized = TRUE;

    Compile_XtEventTable( events );
    CompileNameValueTable( modifiers );
    CompileNameValueTable( buttonNames );
    CompileNameValueTable( notifyModes );
    CompileNameValueTable( notifyDetail );
    CompileNameValueTable( visibilityNotify );
    CompileNameValueTable( circulation );
    CompileNameValueTable( propertyChanged );
} 
