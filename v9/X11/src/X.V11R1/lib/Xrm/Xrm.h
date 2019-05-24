/* $Header: Xrm.h,v 1.1 87/09/12 12:27:30 toddb Exp $ */
/* $Header: Xrm.h,v 1.1 87/09/12 12:27:30 toddb Exp $ */
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

extern void XrmPutResource(); /*quarks, type, val, rdb*/
    /* XrmQuarkList      quarks;     */
    /* XrmRepresentation type;       */
    /* XrmValuePtr		val;	    */
    /* XrmResourceDataBase *rdb; */

extern void XrmGetResource(); /* screen, names, classes, destType, val, rdb*/
    /* Screen		*screen;    */
    /* XrmNameList	names;      */
    /* XrmClassList 	classes;    */
    /* XrmRepresentation destType;   */
    /* XrmValue		*val;       */
    /* XrmResourceDataBase *rdb; */

extern void XrmGetSearchList(); /* names, classes, rdb, searchList */
    /* XrmNameList   names;		    */
    /* XrmClassList  classes;		    */
    /* XrmResourceDataBase *rdb; */
    /* SearchList   searchList;   /* RETURN */

extern void XrmGetSearchResource();
/* screen, searchList, name, class, type, pVal */
    /* Screen	    *screen;		    */
    /* SearchList   searchList;		    */
    /* XrmName       name;		    */
    /* XrmClass      class;		    */
    /* XrmAtom       type;		    */
    /* XrmValue     *pVal;        /* RETURN */
#endif
