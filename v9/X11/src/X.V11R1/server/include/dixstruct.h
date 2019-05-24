
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
/* $Header: dixstruct.h,v 1.7 87/09/11 07:49:48 toddb Exp $ */
#ifndef DIXSTRUCT_H
#define DIXSTRUCT_H

#include "dix.h"
#include "resource.h"
#include "cursor.h"
#include "gc.h"
#include "pixmap.h"

/*
 * 	direct-mapped hash table, used by resource manager to store
 *      translation from client ids to server addresses.
 */

typedef struct _TimeStamp {
    unsigned long	months;			/* really ~49.7 days */
    unsigned long	milliseconds;
} TimeStamp;

#define MAX_REQUEST_LOG 100

typedef struct _Client {
    int index;
    Mask clientAsMask;
    pointer requestBuffer;
    pointer osPrivate;			/* for OS layer, including scheduler */
    Bool swapped;
    void (* pSwapReplyFunc)();		
    int	errorValue;
    int sequence;
    int closeDownMode;
    int clientGone;
    int noClientException;       /* this client died or needs to be killed*/
    DrawablePtr lastDrawable;
    int lastDrawableID;
    GCPtr lastGC;
    int lastGCID;    
    pointer *saveSet;
    int numSaved;
    int requestLog[MAX_REQUEST_LOG];
    int requestLogIndex;
    pointer screenPrivate[MAXSCREENS];
} ClientRec;

extern TimeStamp currentTime;
extern void CloseDownClient();

extern TimeStamp ClientTimeToServerTime();

#endif /* DIXSTRUCT_H */
