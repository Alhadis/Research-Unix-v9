/* $Header: resource.h,v 1.1 87/09/11 07:49:54 toddb Exp $ */
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
#ifndef RESOURCE_H
#define RESOURCE_H 1

/*****************************************************************
 * STUFF FOR RESOURCES 
 *****************************************************************/

/* types for Resource routines */

#define RT_COLORMAP         1<<0
#define RT_FONT             1<<1
#define RT_CURSOR           1<<2
#define RT_PIXMAP           1<<3
#define RT_WINDOW           1<<4
#define RT_DRAWABLE	    (RT_WINDOW | RT_PIXMAP)
#define RT_GC               1<<5
#define RT_FAKE		    1<<6
#define RT_VISUALID         1<<7
#define RT_CMAPENTRY        1<<8
#define RT_LASTPREDEF	    RT_CMAPENTRY
#define RT_ANY		    0xFFFF

/* classes for Resource routines */

#define RC_CORE		    0
#define RC_NONE		    1
#define RC_LASTPREDEF	    RC_NONE

/* bits and fields within a resource id */
#define CLIENTOFFSET 20					/* client field */
#define RESOURCE_ID_MASK	0x7FFFF			/* low 19 bits */
#define CLIENT_BITS(id) ((id) & 0xfff00000)		/* hi 12 bits */
#define CLIENT_ID(id) (CLIENT_BITS(id) >> CLIENTOFFSET)	/* hi 12 bits */
#define SERVER_BIT		0x80000			/* 20th bit reserved */
#define SERVER_BIT_SHIFT	19			/* 20th bit reserved */
#define FAKE_CLIENT_ID(c, i) FakeClientID(c)

/* Invalid resource id */
#define INVALID	(-1)
#endif /* RESOURCE_H */
