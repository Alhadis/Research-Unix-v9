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
#ifndef FONT_H
#define FONT_H 1
#define FONT_FILE_VERSION	4

#include "servermd.h"

#define NullCharInfo ((CharInfoPtr)0)
#define NullFontInfo ((FontInfoPtr)0)
#define LeftToRight 0
#define RightToLeft 1
#define BottomToTop 2
#define TopToBottom 3
/*
 * for linear char sets
 */
#define n1dChars(pfi) ((pfi)->lastCol - (pfi)->firstCol + 1)
#define chFirst firstCol	/* usage:  pfi->chFirst */
#define chLast lastCol		/* usage:  pfi->chLast */

/*
 * for 2D char sets
 */
#define n2dChars(pfi)	(((pfi)->lastCol - (pfi)->firstCol + 1) * \
			 ((pfi)->lastRow - (pfi)->firstRow + 1))

/*
 * Some macros that font converters and font i/o routines will need.
 */

#define	GLWIDTHPIXELS(pci) \
	((pci)->metrics.rightSideBearing - (pci)->metrics.leftSideBearing)
#define	GLHEIGHTPIXELS(pci) \
 	((pci)->metrics.ascent + (pci)->metrics.descent)


/*
 * the following macro definitions describe a font file image in memory
 */
#define ADDRCharInfoRec( pfi)	\
	((CharInfoRec *) &(pfi)[1])

#define ADDRCHARGLYPHS( pfi)	\
	(((char *) &(pfi)[1]) + BYTESOFCHARINFO(pfi))
#define ADDRXTHISCHARINFO( pf, ch ) \
        ((CharInfoRec *) &((pf)->pCI[ch]))


/*
 * pad out glyphs to a CARD32 boundary
 */
#define ADDRXFONTPROPS( pfi)  \
	((DIXFontProp *) ((char *)ADDRCHARGLYPHS( pfi) + BYTESOFGLYPHINFO(pfi)))

#define ADDRSTRINGTAB( pfi)  \
	((char *)ADDRXFONTPROPS( pfi) + BYTESOFPROPINFO(pfi))

#define	BYTESOFFONTINFO(pfi)	(sizeof(FontInfoRec))
#define BYTESOFCHARINFO(pfi)	(sizeof(CharInfoRec) * n2dChars(pfi))
#define	BYTESOFPROPINFO(pfi)	(sizeof(FontPropRec) * (pfi)->nProps)
#define	BYTESOFSTRINGINFO(pfi)	((pfi)->lenStrings)
#define	BYTESOFGLYPHINFO(pfi)	(((pfi)->maxbounds.byteOffset+3) & ~0x3)
 
#define	GLYPHWIDTHBYTES(pci)	(((GLYPHWIDTHPIXELS(pci))+7) >> 3)
#define	GLYPHHEIGHTPIXELS(pci)	(pci->metrics.ascent + pci->metrics.descent)
#define	GLYPHWIDTHPIXELS(pci)	(pci->metrics.rightSideBearing \
				    - pci->metrics.leftSideBearing)
#define GLWIDTHPADDED( bc)	((bc+7) & ~0x7)

#if GLYPHPADBYTES == 0 || GLYPHPADBYTES == 1
#define	GLYPHWIDTHBYTESPADDED(pci)	(GLYPHWIDTHBYTES(pci))
#endif

#if GLYPHPADBYTES == 2
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+1) & ~0x1)
#endif

#if GLYPHPADBYTES == 4
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+3) & ~0x3)
#endif

#if GLYPHPADBYTES == 8 /* for a cray? */
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+7) & ~0x7)
#endif

typedef struct _FontProp *FontPropPtr;
typedef struct _CharInfo *CharInfoPtr;
typedef struct _FontInfo *FontInfoPtr;
typedef unsigned int DrawDirection;
typedef struct _ExtentInfo *ExtentInfoPtr;


#endif /* FONT_H */
