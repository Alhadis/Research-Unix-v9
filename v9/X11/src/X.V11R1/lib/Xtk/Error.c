#ifndef lint
static char rcsid[] = "$Header: Error.c,v 1.3 87/09/11 21:19:06 haynes Rel $";
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
#include <stdio.h>
#include "Intrinsic.h"

void _XtError (message)
    String message;
{
    extern void exit();
    (void) fprintf(stderr, "X Toolkit Error: %s\n", message);
    exit(1);
}

void _XtWarning (message)
    String message;
{ (void) fprintf(stderr, "X Toolkit Warning: %s\n", message); }

static void (*errorFunction)() = _XtError;
static void (*warningFunction)() = _XtWarning;

void XtError(message) String message; { (*errorFunction)(message); }

void XtWarning(message) String message; { (*warningFunction)(message); }

void XtSetErrorHandler(handler)
    register void (*handler)();
{
    if (handler != NULL) errorFunction = handler;
    else errorFunction = _XtError;
}


void XtSetWarningHandler(handler)
    register void (*handler)();
{
    if (handler != NULL) warningFunction = handler;
    else warningFunction = _XtWarning;
}


