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
#ifndef FONTSTRUCT_H
#define FONTSTRUCT_H 1
#include "font.h"
#include "misc.h"

/*
 * This file describes the Server Natural Font format.
 * SNF fonts are both CPU-dependent and frame buffer bit order dependent.
 * This file is used by:
 *	1)  the server, to hold font information read out of font files.
 *	2)  font converters
 *
 * Each font file contains the following
 * data structures, with no padding in-between.
 *
 *	1)  The XFONTINFO structure
 *		hand-padded to a two-short boundary.
 *
 *	2)  The XCHARINFO array
 *		indexed directly with character codes, both on disk
 *		and in memory.
 *
 *	3)  Character glyphs
 *		padded in the server-natural way, and
 *		ordered in the device-natural way.
 *		End of glyphs padded to 32-bit boundary.
 *
 *	4)  nProps font properties
 *
 *	5)  a sequence of null-terminated strings, for font properties
 */


typedef struct _FontProp { 
	CARD32	name;		/* offset of string */
	INT32	value;		/* number or offset of string */
	Bool	indirect;	/* value is a string offset */
} FontPropRec;

typedef struct _CharInfo {
    xCharInfo	metrics;	/* info preformatted for Queries */
    unsigned	byteOffset:24;	/* byte offset of the raster from pGlyphs */
    Bool	exists:1;	/* true iff glyph exists for this char */
    unsigned	pad:7;		/* must be zero for now */
} CharInfoRec;

/*
 * maxbounds.byteOffset is the total number of bytes in the glyph array,
 * and maxbounds.bitOffset is total width of the unpadded font.
 */
typedef struct _FontInfo {
    unsigned int	version1;   /* version stamp */
    unsigned int	allExist;
    unsigned int	drawDirection;
    unsigned int	noOverlap;
    unsigned int	constantMetrics;
    unsigned int	terminalFont;	/* constant metrics &&
					   width of glyph == width of char &&
					   height of glyph == height of font
					*/
    unsigned int	linear:1;	/* true if nRows == 0 */
    unsigned int	padding:31;
    unsigned int	firstCol;
    unsigned int	lastCol;
    unsigned int	firstRow;
    unsigned int	lastRow;
    unsigned int	nProps;
    unsigned int	lenStrings; /* length in bytes of string table */
    unsigned int	chDefault;  /* default character */ 
    unsigned int	fontDescent; /* minimum for quality typography */
    unsigned int	fontAscent;  /* minimum for quality typography */
    CharInfoRec		minbounds;  /* MIN of glyph metrics over all chars */
    CharInfoRec		maxbounds;  /* MAX of glyph metrics over all chars */
    unsigned int	pixDepth;   /* intensity bits per pixel */
    unsigned int	glyphSets;  /* number of sets of glyphs, for
ONT					    sub-pixel positioning */
    unsigned int	version2;   /* version stamp double-check */
} FontInfoRec;

typedef struct _ExtentInfo {
    DrawDirection	drawDirection;
    int			fontAscent;
    int			fontDescent;
    int			overallAscent;
    int			overallDescent;
    int			overallWidth;
    int			overallLeft;
    int			overallRight;
} ExtentInfoRec;

#endif /* FONTSTRUCT_H */

