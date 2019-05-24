/*
* $Header: TMprivate.h,v 1.7 87/09/11 21:24:35 haynes Rel $
*/

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
 * TMprivate.h - Header file private to translation management
 * 
 * Author:	Charles Haynes
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Sat Aug 29 1987
 */

typedef short ModifierMask;

typedef struct _ActionsRec {
    char * token;		/* string token for procedure name */
    void (*proc)();		/* pointer to procedure to call */
    char **param;		/* pointer to array of params */
    unsigned long paramNum;	/* number of params */
    struct _ActionsRec *next;	/* next action to perform */
} ActionRec, *ActionPtr;

typedef struct _EventSeqRec {
    char *str;			/* ascii rep of text. */
    short modifiers;		/* shift, ctrl, meta... */
    short modifierMask;		/* shift, ctrl, meta... */
    unsigned long eventType;	/* key pressed, button released... */        
    unsigned long eventCodeMask;/* which bits of Code are valid? */
    unsigned long eventCode;	/* key or button number */
    struct _EventSeqRec *next;	/* next event on line */
    ActionPtr actions;		/* r.h.s.   list of actions to perform */
} EventSeqRec, *EventSeqPtr;

typedef struct _StateRec {
    int index;			/* index of event into EventObj table */
    ActionPtr actions;		/* rhs   list of actions to perform */
    struct _StateRec *nextLevel;/* the next level points to the next event
				   in one event sequence */
    struct _StateRec *next;	/* points to other event state at same level */
}  StateRec, *StatePtr;


typedef struct _EventObjRec {
    unsigned long eventType;	/* key pressed, button released... */        
    unsigned long eventCodeMask;/* which bits of eventCode are valid? */
    unsigned long eventCode;	/* key or button number */
    short modifiers;		/* shift, ctrl, meta... */
    short modifierMask;		/* shift, ctrl, meta... */
    StatePtr state;		/* pointer to linked lists of state info */
} EventObjRec, *EventObjPtr;   


extern EventObjPtr EventMapObjectCreate();
extern EventObjPtr EventMapObjectGet();
extern EventObjPtr EventMapObjectSet();

typedef XrmQuark XtAction;

typedef struct _TranslationData {
    unsigned int	numEvents;
    unsigned int	eventTblSize;
    EventObjPtr		eventObjTbl;
    unsigned long	clickTime;
} TranslationData;

extern void TranslateEvent();

/* $Log:	TMprivate.h,v $
 * Revision 1.7  87/09/11  21:24:35  haynes
 * ship it. clean up copyright, add rcs headers.
 * 
 * Revision 1.6  87/09/11  17:25:38  haynes
 * remove event index, no one else needs.
 * 
 * Revision 1.5  87/09/11  11:35:25  susan
 * Added detail field ANY
 * 
 * Revision 1.4  87/09/10  14:39:42  haynes
 * major renaming cataclysm, de-linted, cleaned up
 * 
 * Revision 1.3  87/08/29  18:39:04  haynes
 * change XtTranslateEvent to TranslateEvent
 * 
 * Revision 1.2  87/08/29  15:05:42  haynes
 * revised public interface, now only contains exported functions and types
 * (really...)
 * 
 * Revision 1.1  87/08/29  14:44:34  haynes
 * Initial revision
 *  */


