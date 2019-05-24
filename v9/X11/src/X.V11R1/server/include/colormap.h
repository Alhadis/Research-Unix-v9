/*
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

*/
/* $Header: colormap.h,v 1.16 87/09/11 07:50:36 toddb Exp $ */
#ifndef CMAP_H
#define CMAP_H 1

/* these follow X.h's AllocNone and AllocAll */
#define CM_PSCREEN 2
#define CM_PWIN	   3
/* Passed internally in colormap.c */
#define REDMAP 0
#define GREENMAP 1
#define BLUEMAP 2
#define PSEUDOMAP 3
#define AllocPrivate (-1)
#define DynamicClass  1


/* Gets the color from a cell as an Rvalue */
#define RRED(cell) (((cell)->fShared) ? ((cell)->co.shco.red->color): \
		    ((cell)->co.local.red))
#define RGREEN(cell) (((cell)->fShared) ? ((cell)->co.shco.green->color): \
		    ((cell)->co.local.green))
#define RBLUE(cell) (((cell)->fShared) ? ((cell)->co.shco.blue->color): \
		    ((cell)->co.local.blue))

/* Gets the color from a cell as an L value */
#define LRED(cell) (((cell)->fShared) ? (&((cell)->co.shco.red->color)) : \
		    (&((cell)->co.local.red)))
#define LGREEN(cell) (((cell)->fShared) ? (&((cell)->co.shco.green->color)) : \
		    (&((cell)->co.local.green)))
#define LBLUE(cell) (((cell)->fShared) ? (&((cell)->co.shco.blue->color)) : \
		    (&((cell)->co.local.blue)))

/* Values for the flags field of a colormap. These should have 1 bit set
 * and not overlap */
#define IsDefault 1
#define AllAllocated 2
#define BeingCreated 4


typedef unsigned long	Pixel;
typedef struct _CMEntry *EntryPtr;
typedef struct _ColormapRec *ColormapPtr;

extern int CreateColormap();
extern unsigned long FindColor();
extern void FreeColormap();
extern int TellNoMap();
extern int TellNewMap();
extern int IsMapInstalled();
extern void CopyFree();
extern void FreeCell();
extern void UninstallColormap();
extern int     AllComp(), RedComp(), GreenComp(), BlueComp();
extern void    FreeClientPixels(), AllocShared();
extern ColormapPtr CreateStaticColormap();
extern void InitializeLinearColormap();

#endif /* CMAP_H */
