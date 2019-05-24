/* $Header: Xrm.h,v 1.1 87/09/11 08:16:09 toddb Exp $ */
/*
 *	sccsid:	@(#)Xrm.h	1.2	2/25/87
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
 * Xrm.h - resource manager private definitions
 * 
 * Author:	haynes
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Sat Jan 24 1987
 */

#ifndef _Xrm_h_
#define _Xrm_h_

typedef struct _XrmHashBucketRec *XrmHashBucket;
typedef XrmHashBucket *XrmHashTable;
typedef XrmHashTable XrmSearchList[];

extern void XrmPutResource(); /*quarks, type, val*/
    /* XrmQuarkList      quarks;     */
    /* XrmRepresentation type;       */
    /* XrmValue		val;	    */

extern void XrmGetResource(); /* dpy, names, classes, destType, val*/
    /* Display		*dpy;	    */
    /* XrmNameList	names;      */
    /* XrmClassList 	classes;    */
    /* XrmRepresentation destType;   */
    /* XrmValue		*val;       */

extern void XrmGetSearchList(); /* names, classes, searchList */
    /* XrmNameList   names;		    */
    /* XrmClassList  classes;		    */
    /* SearchList   searchList;   /* RETURN */

extern void XrmGetSearchResource();
/* dpy, searchList, name, class, type, pVal */
    /* Display	    *dpy;		    */
    /* SearchList   searchList;		    */
    /* XrmName       name;		    */
    /* XrmClass      class;		    */
    /* XrmAtom       type;		    */
    /* XrmValue     *pVal;        /* RETURN */
#endif
