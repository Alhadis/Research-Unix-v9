#ifndef lint
static char rcsid[] = "$Header: TMstate.c,v 1.9 87/09/11 21:24:37 haynes Rel $";
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
/* TMstate.c -- maintains the state table of actions for the translation 
 *              manager.
 */

#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "TM.h"
#include "TMprivate.h"

extern char *strcpy();

static void FreeActionRecs(action)
  ActionPtr action;
{
    int i;
    if (action->next != NULL)
        FreeActionRecs(action->next);
    if (action->param != NULL)
        for (i=0; i<action->paramNum; i++)
            XtFree(action->param[i]);
    XtFree(action->token);
    XtFree((char *)action->param);
    XtFree((char *)action);
}


static void FreeStateRecs(state)
  StatePtr state;
{
    if (state->next != NULL)
	FreeStateRecs(state->next);
    if (state->nextLevel != NULL)
	FreeStateRecs(state->nextLevel);
    if (state->actions != NULL)
        FreeActionRecs(state->actions);
    XtFree((char *)state->actions);
    XtFree((char *)state->next);
    XtFree((char *)state->nextLevel);
}

static void PrintModifiers(mask, mod)
    unsigned long mask, mod;
{
    if (mask & ShiftMask)
	(void) printf("<%sShift>", ((mod & ShiftMask) ? "" : "~"));
    if (mask & ControlMask)
	(void) printf("<%sCtrl>", ((mod & ControlMask) ? "" : "~"));
    if (mask & LockMask)
	(void) printf("<%sLock>", ((mod & LockMask) ? "" : "~"));
    if (mask & Mod1Mask)
	(void) printf("<%sMeta>", ((mod & Mod1Mask) ? "" : "~"));
    if (mask & Mod2Mask)
	(void) printf("<%sMod2>", ((mod & Mod2Mask) ? "" : "~"));
    if (mask & Mod3Mask)
	(void) printf("<%sMod3>", ((mod & Mod3Mask) ? "" : "~"));
    if (mask & Mod4Mask)
	(void) printf("<%sMod4>", ((mod & Mod4Mask) ? "" : "~"));
    if (mask & Mod5Mask)
	(void) printf("<%sMod5>", ((mod & Mod5Mask) ? "" : "~"));
}

static void PrintEvent(event, endStr)
    unsigned long event;
    char * endStr;
{
    (void) strcpy(endStr, "");
    switch (event) {
	case KeyPressMask:
	    (void) printf("<Key>");
	    break;
	case KeyReleaseMask:
	    (void) printf("<Key>");
	    break;
	case ButtonPressMask:
	    (void) printf("<Btn");
	    (void) strcpy(endStr, "Down>");
	    break;
	case ButtonReleaseMask:
	    (void) printf("<Btn");
	    (void) strcpy(endStr, "Up>");
	    break;
    }
}

static void PrintCode(code)
    unsigned long code;
{
    char ch;
    ch = code - 'a' + 'A';
    if (ch >= 'A' && ch <= 'Z')
	(void) printf("%c",ch);
    else
        (void) printf("%d", code);
}

static void PrintActions(ev, code, mask, modif, actions, j)
    unsigned long ev[], mask[], modif[], code[];
    ActionPtr actions;
    int j;
{
    char endStr[20];
    int i;
    for (i=0; i<j; i++) {
	PrintModifiers(mask[i], modif[i]);
	PrintEvent(ev[i], endStr);
	PrintCode(code[i]);
        (void) printf("%s ",endStr);
    }
    while (actions != NULL && actions->token != NULL) {
        (void) printf("---------> action = %s\n", actions->token);
	actions = actions->next;
    }
    (void) printf("\n");
}

/*
 * there are certain cases where you want to ignore the event and stay
 * in the same state.
 */
static Boolean SpecialCase(event)
  EventSeqPtr event;   
{
    if (event->eventType == MotionNotify || event->eventType == ButtonPress ||
	event->eventType == ButtonRelease)
	    return TRUE;
    else
	    return FALSE;
}


static int FindEvent(translations, eventSeq) 
  _XtTranslations translations;
  EventSeqPtr eventSeq;
{
    EventObjPtr eventTbl = translations->eventObjTbl;
    int i;

    for (i=0; i < translations->numEvents; i++) {
        if (
	    (eventTbl[i].eventType	== eventSeq->eventType) &&
            (eventTbl[i].eventCode	== eventSeq->eventCode) &&
	    (eventTbl[i].eventCodeMask	== eventSeq->eventCodeMask) &&
	    (eventTbl[i].modifiers	== eventSeq->modifiers) &&
	    (eventTbl[i].modifierMask	== eventSeq->modifierMask)
	   )
		return(i);
   }
    return(-1);
}


static int MatchEvent(translations, eventSeq) 
  _XtTranslations translations;
  EventSeqPtr eventSeq;
{
    EventObjPtr eventTbl = translations->eventObjTbl;
    int i;

    for (i=0; i < translations->numEvents; i++) {
        if ((eventTbl[i].eventType == eventSeq->eventType) &&
            (eventTbl[i].eventCode ==
		(eventTbl[i].eventCodeMask & eventSeq->eventCode)) &&
	    (eventTbl[i].modifiers ==
		(eventTbl[i].modifierMask & eventSeq->modifiers))
	   ) 
		return(i);
   }
    return(-1);
}


static EventObjPtr  CreateStates(translations, index, eventSeq)
    _XtTranslations translations;
    int index;
    EventSeqPtr eventSeq;
{
    EventObjPtr eventObjTbl = translations->eventObjTbl;
    StatePtr state;
    Boolean found = FALSE;
    ActionPtr actions = eventSeq->actions;

    if (eventObjTbl[index].state == NULL) {  
	eventObjTbl[index].state = (StatePtr) XtMalloc((unsigned)sizeof(StateRec));
        state = eventObjTbl[index].state;
        state->index = index;
        state->nextLevel = NULL;
        state->next = NULL;
        state->actions = NULL; ;
    } else {
        state = eventObjTbl[index].state;
	do {
	    found = FALSE;
	    if (eventSeq->next != NULL && state->nextLevel != NULL) {
		eventSeq = eventSeq->next;
		index = FindEvent(translations, eventSeq);
		state = state->nextLevel;
		if (state->index == index) 
		    found = TRUE;
		while (state->next != NULL && !found) {
	            state = state->next;
		    if (state->index == index) 
		        found = TRUE;
		}
		if (!found) {
		    state->next = (StatePtr) XtMalloc((unsigned)sizeof(StateRec));
		    state = state->next;
		    state->index = index;
		    state->nextLevel = NULL;
		    state->next = NULL;
		    state->actions = NULL;
		}
	    }
	} while (found);
    }
    while (eventSeq->next != NULL) {
	eventSeq = eventSeq->next;
 	index = FindEvent(translations, eventSeq);
	state->nextLevel = (StatePtr) XtMalloc((unsigned)sizeof(StateRec));
	state = state->nextLevel;
	state->index = index;
	state->nextLevel = NULL;
	state->next = NULL;
	state->actions = NULL;
    }
    if (state->actions != NULL) 
	FreeActionRecs(state->actions);
    state->actions = actions;

    return eventObjTbl;
}


/*** Public procedures ***/
EventObjPtr EventMapObjectCreate(translations, eventSeq)
  _XtTranslations	translations;
  EventSeqPtr eventSeq;
{
    EventObjPtr new;

    if (FindEvent(translations, eventSeq) >= 0)
	return translations->eventObjTbl;

    if (translations->numEvents == translations->eventTblSize) {
        translations->eventTblSize += 100;
	translations->eventObjTbl = (EventObjPtr) XtRealloc(
	    (char *)translations->eventObjTbl, 
	    translations->eventTblSize*sizeof(EventObjRec));
    }

    new = &translations->eventObjTbl[translations->numEvents];

    new->eventType	= eventSeq->eventType;
    new->eventCodeMask	= eventSeq->eventCodeMask;
    new->eventCode	= eventSeq->eventCode;
    new->modifierMask	= eventSeq->modifierMask;
    new->modifiers	= eventSeq->modifiers;
    new->state		= NULL;

    translations->numEvents++;
    return translations->eventObjTbl;
}


EventObjPtr EventMapObjectGet(translations, eventSeq)
  _XtTranslations translations;
  EventSeqPtr eventSeq;
{
    EventObjPtr eventTbl = translations->eventObjTbl;
    int index;
    if ((index = FindEvent(translations, eventSeq)) < 0)
	return NULL;
    else
        return &eventTbl[index];
}


EventObjPtr EventMapObjectSet(translations, eventSeq)
  _XtTranslations translations;
  EventSeqPtr eventSeq;
{
    EventObjPtr eventTbl = translations->eventObjTbl;
    int index;
    if ((index = FindEvent(translations, eventSeq)) >= 0)
        eventTbl = CreateStates(translations, index, eventSeq);
    return eventTbl;
}


/* ARGSUSED */
void TranslateEvent(w, closure, event)
  Widget w;
  Opaque closure;
  register XEvent *event;
{
    static unsigned long upTime=0;
    static Boolean buttonUp = FALSE;
    static StatePtr curState = NULL;
    StatePtr oldState;
    EventSeqRec curEvent;
    int index;
    ActionPtr actions;
    Boolean specialCase;

    oldState = 0;
    specialCase = FALSE;
    curEvent.eventCodeMask = 0;
    curEvent.eventCode = 0;
    curEvent.modifierMask = 0;
    curEvent.modifiers = 0;
    curEvent.eventType = event->type;
    switch (event->type) {
	case KeyPress:
	case KeyRelease:
	    buttonUp = FALSE;
	    curEvent.modifiers = event->xkey.state;
	    event->xkey.state = 0;
	    curEvent.eventCode = XLookupKeysym(&event->xkey, 0);
	    event->xkey.state = curEvent.modifiers;
	    break;
	case ButtonPress:
	    if (buttonUp && curState != NULL) 
		if ((unsigned long)
	            (upTime - event->xbutton.time) >
		    w->core.translations->clickTime)
		    curState = NULL;
	    buttonUp = FALSE;
 	    curEvent.eventCode = event->xbutton.button;
	    curEvent.modifiers = event->xbutton.state;
	    break;
	case ButtonRelease:
	    buttonUp = TRUE;
	    upTime = event->xbutton.time;
	    curEvent.eventCode = event->xbutton.button;
	    curEvent.modifiers = event->xbutton.state;
	    break;
	case MotionNotify:
	    buttonUp = FALSE;
	    curEvent.modifiers = event->xmotion.state;
	    break;
	case EnterNotify:
	case LeaveNotify:
	    buttonUp = FALSE;
	    curEvent.modifiers = event->xcrossing.state;
	    break;
	default:
	    buttonUp = FALSE;
	    break;
    }
    if (curState != NULL) {	/* check the current level */
	index = MatchEvent(w->core.translations, &curEvent);
	oldState = curState;
	while (curState != NULL && curState->index != index)
	    curState = curState->next;
	if (curState == NULL)
	    if (SpecialCase(&curEvent)) {
	        curState = oldState;
	        specialCase = TRUE;
	    } else {  /* nothing at level but you performed an action on the
			 last event---> start over with this new event. */
		if (oldState->actions != NULL) {
		    curState = oldState;
		    index = MatchEvent(w->core.translations, &curEvent);
        	    curState = w->core.translations->eventObjTbl[index].state;
		}
           }
    } else {
	index = MatchEvent(w->core.translations, &curEvent);
	if (index == -1) return;
        curState = w->core.translations->eventObjTbl[index].state;
    }
    if (curState != NULL && !specialCase) {
	actions = curState->actions;
	while (actions != NULL)  { /* perform any actions */
	    if (actions->proc != NULL)
			/*!!!!! should have params here */
		(*(actions->proc))(w, event);
	    actions = actions->next;
        }
        curState = curState->nextLevel;
    }
}

void TranslateTableFree(translations)
    _XtTranslations translations;
{
    EventObjPtr tbl = translations->eventObjTbl;
    int i;

    /* !!! ref count this, it may be shared */

    for (i=0; i<translations->numEvents; i++) {
	if (tbl[i].state != NULL) FreeStateRecs(tbl[i].state);
    }
    XtFree((char *)tbl);
    XtFree((char *)translations);
}


void TranslateTablePrint(translations)
    _XtTranslations translations;
{
    EventObjPtr tbl = translations->eventObjTbl;
    int i, j;
    unsigned long ev[100], code[100], mask[100], modif[100];
    StatePtr stack[100], state;
	
    for (i=0; i<translations->numEvents; i++) {
	j=0;
	if (tbl[i].state != NULL) {
	    state = tbl[i].state;
	    stack[j] = tbl[i].state;
	    ev[j] = tbl[i].eventType;
	    code[j] = tbl[i].eventCode;
	    mask[j] = tbl[i].modifierMask;
	    modif[j++] = tbl[i].modifiers;
            if (state->actions != NULL)
	        PrintActions(ev, code, mask, modif, state->actions, j);
	    do {
	        do {
	            while (state->nextLevel != NULL) {
	                state = state->nextLevel;
	                stack[j] = state;
	                ev[j] = tbl[state->index].eventType;
		        code[j] = tbl[state->index].eventCode;
			mask[j] =  tbl[state->index].modifierMask;
	                modif[j++] = tbl[state->index].modifiers;
                    	if (state->actions != NULL)
	                    PrintActions(
				ev, code, mask, modif, state->actions, j);
	            }
	            j--;
	            if (state->next != NULL) {
	            	state = state->next;
	            	stack[j] = state;
	            	ev[j] = tbl[state->index].eventType;
	            	code[j] = tbl[state->index].eventCode;
	            	mask[j] = tbl[state->index].modifierMask;
	            	modif[j++] = tbl[state->index].modifiers;
                    	if (state->actions != NULL)
	                    PrintActions(
				ev, code, mask, modif, state->actions, j);
	            }
	     	} while (state->next != NULL &&  state->nextLevel != NULL);
	    	j--;
	    	state = stack[j];
	    	if (state->next != NULL) {
	            state = state->next;
		    stack[j] = state;
	            ev[j] = tbl[state->index].eventType;
	            code[j] = tbl[state->index].eventCode;
	            mask[j] = tbl[state->index].modifierMask;
	            modif[j++] = tbl[state->index].modifiers;
                    if (state->actions != NULL)
	                PrintActions(ev, code, mask, modif, state->actions, j);
	        } else
		    j--;
	    } while (j-1>0);
	}
    }
}
