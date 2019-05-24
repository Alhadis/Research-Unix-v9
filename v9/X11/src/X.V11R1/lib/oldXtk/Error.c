/* $Header: Error.c,v 1.1 87/09/11 07:57:11 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Error.c	1.4	2/25/87";
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

#define num_error_codes 3

char *XtErrorList[num_error_codes + 1] = {
	/* No error		*/	"",
	/* XtNOMEM		*/	"insufficient resources, bad alloc",
	/* XCNOENT		*/      "No entry in table ",
	/* XtFOPEN              */	"fopen failed ",
};


char *XtErrDescrip (code)
    register int code;
{
    if (code <= num_error_codes && code > 0)
	return (XtErrorList[code]);
    return("Unknown error");
}

int _XtError (errorCode)
    int errorCode;
{
    extern void exit();
    (void) fprintf(stderr, "X Toolkit Error: %s\n", XtErrDescrip (errorCode));
    (void) fprintf(stderr, "         Reference Count: %d\n", XtreferenceCount);
    exit(1);
}


void XtErrorHandler(handler)
    register int (*handler)();
{
    if (handler != NULL) {
	XtErrorFunction = handler;
    }
    else {
	XtErrorFunction = _XtError;
    }
}


