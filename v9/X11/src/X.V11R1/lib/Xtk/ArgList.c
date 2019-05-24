#ifndef lint
static char rcsid[] = "$Header: ArgList.c,v 1.2 87/09/11 21:18:27 haynes Rel $";
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
#include	"Xlib.h"
#include	"Intrinsic.h"
#include	<stdio.h>

void PrintArgList(args, argCount)
    ArgList	args;
    int argCount;
{
    for (; --argCount >= 0; args++) {
	(void) printf("name: %s, value: 0x%x\n", args->name, args->value);
    }
}

/*
 * This routine merges two arglists. It does NOT check for duplicate entries.
 */

ArgList XtMergeArgLists(args1, argCount1, args2, argCount2)
    register ArgList args1;
    register int     argCount1;
    register ArgList args2;
    register int     argCount2;
{
    register ArgList result, args;

    result = (ArgList) XtCalloc((unsigned) argCount1 + argCount2, sizeof(Arg));

    for (args = result; --argCount1 >= 0; args++, args1++)
    	*args = *args1;
    for (             ; --argCount2 >= 0; args++, args2++)
    	*args = *args2;

    return result;
}

 
