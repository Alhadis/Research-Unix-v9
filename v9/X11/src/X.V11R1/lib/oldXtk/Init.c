/* $Header: Init.c,v 1.1 87/09/11 07:57:58 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Init.c	1.2	2/25/87";
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


#include "Xlib.h"
#include "Intrinsic.h"

extern int _XtError();

int XtreferenceCount;
int (*XtErrorFunction)();

extern void  QuarkInitialize();
extern void  XrmInitialize();
extern void  ResourceListInitialize();
extern void  EventInitialize();
extern void  ActionsInitialize();
extern void  CursorsInitialize();
extern void  GCManagerInitialize();
extern void  GeometryInitialize();

void XtInitialize()
{
    /* 
     * For Error handling.
     */
    XtErrorFunction = _XtError;
    XtreferenceCount = 1;

    /* Resource management initialization */
    QuarkInitialize();
    XrmInitialize();
    ResourceListInitialize();

    /* Other intrinsic intialization */
    EventInitialize();
    ActionsInitialize();
    CursorsInitialize();
    GCManagerInitialize();
    GeometryInitialize();
}
