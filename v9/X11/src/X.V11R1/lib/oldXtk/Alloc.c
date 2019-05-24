/* $Header: Alloc.c,v 1.1 87/09/11 07:57:05 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Alloc.c	1.5	2/24/87";
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

/*
 * X Toolkit Memory Allocation Routines
 */
extern char *malloc(), *realloc(), *calloc();
extern void exit(), free();
#include "Xlib.h"
#include "Intrinsic.h"

char *XtMalloc(size)
    unsigned size;
{
    char *ptr;

    if ((ptr = malloc(size)) == NULL) 
	XtErrorFunction(XtNOMEM);
    return(ptr);
}

char *XtRealloc(ptr, size)
    char     *ptr;
    unsigned size;
{
   if (ptr == NULL) 
	return(XtMalloc(size));
   else if ((ptr = realloc(ptr, size)) == NULL) 
	XtErrorFunction(XtNOMEM);
    return(ptr);
}

char *XtCalloc(num, size)
    unsigned num, size;
{
    char *ptr;

    if ((ptr = calloc(num, size)) == NULL) 
	XtErrorFunction(XtNOMEM);
    return(ptr);
}

void XtFree(ptr)
    char *ptr;
{
   if (ptr != NULL) free(ptr);
}

