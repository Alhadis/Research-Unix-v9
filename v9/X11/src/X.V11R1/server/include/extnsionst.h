/* $Header: extnsionst.h,v 1.4 87/08/31 19:58:18 toddb Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
#ifndef EXTENSIONSTRUCT_H
#define EXTENSIONSTRUCT_H 
#include "extension.h"
typedef struct _ExtensionEntry {
    int index;
    void (* CloseDown)();	/* called at server shutdown */
    char *name;               /* extension name */
    int base;                 /* base request number */
    int eventBase;            
    int eventLast;
    int errorBase;
    int errorLast;
    pointer extPrivate;
} ExtensionEntry;

typedef void (* ExtensionLookupProc)();

typedef struct _ProcEntry {
    char *name;
    ExtensionLookupProc proc;
} ProcEntryRec, *ProcEntryPtr;

typedef struct _ScreenProcEntry {
    int num;
    ProcEntryPtr procList;
} ScreenProcEntry;

#define    InsertGCI(pGC, pGCI, order, pPrevGCI)    \
	   order(pGC,pGCI,pPrevGCI)

#define GCI_FIRST(pGC,pGCI,dummy)\
    {					    \
    pGCI->pNextGCInterest = pGC->pNextGCInterest;\
    pGCI->pLastGCInterest = (GCInterestPtr)&pGC->pNextGCInterest; \
    pGC->pNextGCInterest->pLastGCInterest = pGCI; \
    pGC->pNextGCInterest=pGCI;		    \
    }					    

#define GCI_MIDDLE(pGC,pGCI,pPrevGCI)\
    {					    \
    pGCI->pNextGCInterest = pPrevGCI->pNextGCInterest;\
    pGCI->pLastGCInterest = (GCInterestPtr)&pPrevGCI->pNextGCInterest; \
    pPrevGCI->pNextGCInterest->pLastGCInterest = pGCI; \
    pPrevGCI->pNextGCInterest=pGCI;		    \
    }

#define GCI_LAST(pGC,pGCI,dummy)\
    {					    \
    pGCI->pNextGCInterest = (GCInterestPtr)&pGC->pNextGCInterest;\
    pGCI->pLastGCInterest = pGC->pLastGCInterest;\
    pGC->pLastGCInterest->pNextGCInterest = pGCI;\
    pGC->pLastGCInterest=pGCI;		     \
    }

#define RemoveGCI(pGCI) \
	pGCI->pNextGCInterest->pLastGCInterest = pGCI->pLastGCInterest;\
	pGCI->pLastGCInterest->pNextGCInterest = pGCI->pNextGCInterest;\
	pGCI->pNextGCInterest = 0;\
	pGCI->pLastGCInterest = 0;

#define    SetGCVector(pGC, VectorElement, NewRoutineAddress, Atom)    \
    pGC->VectorElement = NewRoutineAddress;

#define    GetGCValue(pGC, GCElement)    (pGC->GCElement)

extern void InitExtensions();
extern int ProcQueryExtension();
extern int ProcListExtensions();
extern ExtensionEntry *AddExtension();
extern ExtensionLookupProc LookupProc();
extern void RegisterProc();
extern void RegisterScreenProc();

/*  List of extension procs go here */

#endif /* EXTENSIONSTRUCT_H */
