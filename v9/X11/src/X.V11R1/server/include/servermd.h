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
#ifndef SERVERMD_H
#define SERVERMD_H 1
/* $Header: servermd.h,v 1.26 87/09/13 00:56:46 toddb Exp $ */

/*
 * The vendor string identifies the vendor responsible for the
 * server executable.
 */
#ifndef VENDOR_STRING
#define VENDOR_STRING "Massachusetts Institute of Technology"
#endif	VENDOR_STRING

/*
 * The vendor release number identifies, for the purpose of submitting
 * traceable bug reports, the release number of software produced
 * by the vendor.
 */
#ifndef VENDOR_RELEASE
#define VENDOR_RELEASE	1
#endif	VENDOR_RELEASE

/*
 * Machine dependent values:
 * GLYPHPADBYTES should be chosen with consideration for the space-time
 * trade-off.  Padding to 0 bytes means that there is no wasted space
 * in the font bitmaps (both on disk and in memory), but that access of
 * the bitmaps will cause odd-address memory references.  Padding to
 * 2 bytes would ensure even address memory references and would
 * be suitable for a 68010-class machine, but at the expense of wasted
 * space in the font bitmaps.  Padding to 4 bytes would be good
 * for real 32 bit machines, etc.  Be sure that you tell the font
 * compiler what kind of padding you want because its defines are
 * kept separate from this.  See server/include/fonts.h for how
 * GLYPHPADBYTES is used.
 *
 * Along with this, you should choose an appropriate value for
 * GETLEFTBITS_ALIGNMENT, which is used in ddx/mfb/maskbits.h.  This
 * constant choses what kind of memory references are guarenteed during
 * font access; either 1, 2 or 4, for byte, word or longword access,
 * respectively.  For instance, if you have decided to to have
 * GLYPHPADBYTES == 4, then it is pointless for you to have a
 * GETLEFTBITS_ALIGNMENT > 1, because the padding of the fonts has already
 * guarenteed you that your fonts are longword aligned.  On the other
 * hand, even if you have chosen GLYPHPADBYTES == 1 to save space, you may
 * also decide that the computing involved in aligning the pointer is more
 * costly than an odd-address access; you choose GETLEFTBITS_ALIGNMENT == 1.
 * 
 * XXX: this code has changed since beta test and only GLYPHPADBYTES == 4
 * has been tested, hence all machines have this same value.
 *
 */

#ifdef vax

#define IMAGE_BYTE_ORDER	LSBFirst        /* Values for the VAX only */
#define BITMAP_BIT_ORDER	LSBFirst
#define	GLYPHPADBYTES		1
#define GETLEFTBITS_ALIGNMENT	4

#else
# ifdef sun

#define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the SUN only */
#define BITMAP_BIT_ORDER	MSBFirst
#define	GLYPHPADBYTES		4
#define GETLEFTBITS_ALIGNMENT	4

# else
#  ifdef apollo

#define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the Apollo only*/
#define BITMAP_BIT_ORDER	MSBFirst
#define	GLYPHPADBYTES		2
#define GETLEFTBITS_ALIGNMENT	4

#  else
#   ifdef ibm032

#define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the RT only*/
#define BITMAP_BIT_ORDER	MSBFirst
#define	GLYPHPADBYTES		1
#define GETLEFTBITS_ALIGNMENT	4
/* ibm pcc doesn't understand pragmas. */

#   endif
#  endif
# endif
#endif

/* size of buffer to use with GetImage, measured in bytes. There's obviously
 * a trade-off between the amount of stack (or whatever ALLOCATE_LOCAL gives
 * you) used and the number of times the ddx routine has to be called.
 * 
 * for a 1024 x 864 bit monochrome screen  with a 32 bit word we get 
 * 8192/4 words per buffer 
 * (1024/32) = 32 words per scanline
 * 2048 words per buffer / 32 words per scanline = 64 scanlines per buffer
 * 864 scanlines / 64 scanlines = 14 buffers to draw a full screen
 */
#define IMAGE_BUFSIZE		8192

/* pad scanline to a longword */
#define BITMAP_SCANLINE_UNIT	32
#define BITMAP_SCANLINE_PAD  32

#define LOG2_BITMAP_PAD		5
#define LOG2_BYTES_PER_SCANLINE_PAD	2

/* 
 *   This returns the number of padding units, for depth d and width w.
 * For bitmaps this can be calculated with the macros above.
 * Other depths require either grovelling over the formats field of the
 * screenInfo or hardwired constants.
 */

typedef struct _PaddingInfo {
	int	scanlinePad;
	int	bitmapPadLog2;
} PaddingInfo;
extern PaddingInfo PixmapWidthPaddingInfo[];

#define PixmapWidthInPadUnits(w, d) \
    (((w) + PixmapWidthPaddingInfo[d].scanlinePad) >> \
	PixmapWidthPaddingInfo[d].bitmapPadLog2)

/*
 *	Return the number of bytes to which a scanline of the given
 * depth and width will be padded.
 */
#define PixmapBytePad(w, d) \
    (PixmapWidthInPadUnits(w, d) << LOG2_BYTES_PER_SCANLINE_PAD)

#endif SERVERMD_H
