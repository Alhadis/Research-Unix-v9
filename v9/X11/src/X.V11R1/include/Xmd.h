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
#ifndef XMD_H
#define XMD_H 1
/* $Header: Xmd.h,v 1.22 87/09/07 18:07:40 toddb Exp $ */
/*
 *  Xmd.h: MACHINE DEPENDENT DECLARATIONS.
 */

/*
 * ibm pcc doesn't understand pragmas.
 */
#if defined(ibm032) && !defined(_pcc_)
pragma on(pointers_compatible);
pragma off(char_default_unsigned);
#endif

/*
 * Bitfield suffixes for the protocol structure elements, if you
 * need them.
 */
#define B16
#define B32

typedef long           INT32;
typedef short          INT16;
typedef char           INT8;

typedef unsigned long CARD32;
typedef unsigned short CARD16;
typedef unsigned char  CARD8;

typedef unsigned long		BITS32;
typedef unsigned short		BITS16;
typedef unsigned char		BYTE;

typedef unsigned char            BOOL;

#endif /* XMD_H */
