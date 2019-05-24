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
/* $Header: maskbits.h,v 1.6 87/09/12 23:46:43 toddb Exp $ */
#include "X.h"
#include "Xmd.h"
#include "servermd.h"

extern int starttab[];
extern int endtab[];
extern int startpartial[];
extern int endpartial[];
extern int rmask[32];
extern int mask[32];


/* the following notes use the following conventions:
SCREEN LEFT				SCREEN RIGHT
in this file and maskbits.c, left and right refer to screen coordinates,
NOT bit numbering in registers.

starttab[n] 
	bits[0,n-1] = 0	bits[n,31] = 1
endtab[n] =
	bits[0,n-1] = 1	bits[n,31] = 0

startpartial[], endpartial[]
	these are used as accelerators for doing putbits and masking out
bits that are all contained between longword boudaries.  the extra
256 bytes of data seems a small price to pay -- code is smaller,
and narrow things (e.g. window borders) go faster.

the names may seem misleading; they are derived not from which end
of the word the bits are turned on, but at which end of a scanline
the table tends to be used.

look at the tables and macros to understand boundary conditions.
(careful readers will note that starttab[n] = ~endtab[n] for n != 0)

-----------------------------------------------------------------------
these two macros depend on the screen's bit ordering.
in both of them x is a screen position.  they are used to
combine bits collected from multiple longwords into a
single destination longword, and to unpack a single
source longword into multiple destinations.

SCRLEFT(dst, x)
	takes dst[x, 32] and moves them to dst[0, 32-x]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	this is a right shift on LSBFirst (forward-thinking)
	machines like the VAX, and left shift on MSBFirst
	(backwards) machines like the 680x0 and pc/rt.

SCRRIGHT(dst, x)
	takes dst[0,x] and moves them to dst[32-x, 32]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	this is a left shift on LSBFirst, right shift
	on MSBFirst.


the remaining macros are cpu-independent; all bit order dependencies
are built into the tables and the two macros above.

maskbits(x, w, startmask, endmask, nlw)
	for a span of width w starting at position x, returns
a mask for ragged bits at start, mask for ragged bits at end,
and the number of whole longwords between the ends.

maskpartialbits(x, w, mask)
	works like maskbits(), except all the bits are in the
	same longword (i.e. (x&0x1f + w) <= 32)

mask32bits(x, w, startmask, endmask, nlw)
	as maskbits, but does not calculate nlw.  it is used by
	mfbGlyphBlt to put down glyphs <= 32 bits wide.

-------------------------------------------------------------------

NOTE
	any pointers passe to the following 4 macros are
	guranteed to be 32-bit aligned.
	The only non-32-bit-aligned references ever made are
	to font glyphs, and those are made with getleftbits()
	and getshiftedleftbits (qq.v.)

getbits(psrc, x, w, dst)
	starting at position x in psrc (x < 32), collect w
	bits and put them in the screen left portion of dst.
	psrc is a longword pointer.  this may span longword boundaries.
	it special-cases fetching all w bits from one longword.

	+--------+--------+		+--------+
	|    | m |n|      |	==> 	| m |n|  |
	+--------+--------+		+--------+
	    x      x+w			0     w
	psrc     psrc+1			dst
			m = 32 - x
			n = w - m

	implementation:
	get m bits, move to screen-left of dst, zeroing rest of dst;
	get n bits from next word, move screen-right by m, zeroing
		 lower m bits of word.
	OR the two things together.

putbits(src, x, w, pdst)
	starting at position x in pdst, put down the screen-leftmost
	w bits of src.  pdst is a longword pointer.  this may
	span longword boundaries.
	it special-cases putting all w bits into the same longword.

	+--------+			+--------+--------+
	| m |n|  |		==>	|    | m |n|      |
	+--------+			+--------+--------+
	0     w				     x     x+w
	dst				pdst     pdst+1
			m = 32 - x
			n = w - m

	implementation:
	get m bits, shift screen-right by x, zero screen-leftmost x
		bits; zero rightmost m bits of *pdst and OR in stuff
		from before the semicolon.
	shift src screen-left by m, zero bits n-32;
		zero leftmost n bits of *(pdst+1) and OR in the
		stuff from before the semicolon.

putbitsrop(src, x, w, pdst, ROP)
	like putbits but calls DoRop with the rasterop ROP (see mfb.h for
	DoRop)

putbitsrrop(src, x, w, pdst, ROP)
	like putbits but calls DoRRop with the reduced rasterop ROP 
	(see mfb.h for DoRRop)

-----------------------------------------------------------------------
	The two macros below are used only for getting bits from glyphs
in fonts, and glyphs in fonts are gotten only with the following two
mcros.
	You should tune these macros toyour font format and cpu
byte ordering.

NOTE
getleftbits(psrc, w, dst)
	get the leftmost w (w<=32) bits from *psrc and put them
	in dst.  this is used by the mfbGlyphBlt code for glyphs
	<=32 bits wide.
	psrc is declared (unsigned char *)

	psrc is NOT guaranteed to be 32-bit aligned.  on  many
	machines this will cause problems, so there are several
	versions of this macro.

	this macro is called ONLY for getting bits from font glyphs,
	and depends on the server-natural font padding.

	for blazing text performance, you want this macro
	to touch memory as infrequently as possible (e.g.
	fetch longwords) and as efficiently as possible
	(e.g. don't fetch misaligned longwords)

getshiftedleftbits(psrc, offset, w, dst)
	used by the font code; like getleftbits, but shifts the
	bits SCRLEFT by offset.
	this is implemented portably, calling getleftbits()
	and SCRLEFT().
	psrc is declared (unsigned char *).
*/

#if (BITMAP_BIT_ORDER == MSBFirst)	/* pc/rt, 680x0 */
#define SCRLEFT(lw, n)	((lw) << (n))
#define SCRRIGHT(lw, n)	((lw) >> (n))
#else					/* vax, intel */
#define SCRLEFT(lw, n)	((lw) >> (n))
#define SCRRIGHT(lw, n)	((lw) << (n))
#endif


#define maskbits(x, w, startmask, endmask, nlw) \
    startmask = starttab[(x)&0x1f]; \
    endmask = endtab[((x)+(w)) & 0x1f]; \
    if (startmask) \
	nlw = (((w) - (32 - ((x)&0x1f))) >> 5); \
    else \
	nlw = (w) >> 5;

#define maskpartialbits(x, w, mask) \
    mask = startpartial[(x) & 0x1f] & endpartial[((x) + (w)) & 0x1f];

#define mask32bits(x, w, startmask, endmask) \
    startmask = starttab[(x)&0x1f]; \
    endmask = endtab[((x)+(w)) & 0x1f];


#define getbits(psrc, x, w, dst) \
if ( ((x) + (w)) <= 32) \
{ \
    dst = SCRLEFT(*(psrc), (x)); \
} \
else \
{ \
    int m; \
    m = 32-(x); \
    dst = (SCRLEFT(*(psrc), (x)) & endtab[m]) | \
	  (SCRRIGHT(*((psrc)+1), m) & starttab[m]); \
}


#define putbits(src, x, w, pdst) \
if ( ((x)+(w)) <= 32) \
{ \
    int tmpmask; \
    maskpartialbits((x), (w), tmpmask); \
    *(pdst) = (*(pdst) & ~tmpmask) | (SCRRIGHT(src, x) & tmpmask); \
} \
else \
{ \
    int m; \
    int n; \
    m = 32-(x); \
    n = (w) - m; \
    *(pdst) = (*(pdst) & endtab[x]) | (SCRRIGHT(src, x) & starttab[x]); \
    *((pdst)+1) = (*((pdst)+1) & starttab[n]) | (SCRLEFT(src, m) & endtab[n]); \
}

#define putbitsrop(src, x, w, pdst, rop) \
if ( ((x)+(w)) <= 32) \
{ \
    int tmpmask; \
    int t1, t2; \
    maskpartialbits((x), (w), tmpmask); \
    t1 = SCRRIGHT((src), (x)); \
    t2 = DoRop(rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
} \
else \
{ \
    int m; \
    int n; \
    int t1, t2; \
    m = 32-(x); \
    n = (w) - m; \
    t1 = SCRRIGHT((src), (x)); \
    t2 = DoRop(rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & endtab[x]) | (t2 & starttab[x]); \
    t1 = SCRLEFT((src), m); \
    t2 = DoRop(rop, t1, *((pdst) + 1)); \
    *((pdst)+1) = (*((pdst)+1) & starttab[n]) | (t2 & endtab[n]); \
}

#define putbitsrrop(src, x, w, pdst, rop) \
if ( ((x)+(w)) <= 32) \
{ \
    int tmpmask; \
    int t1, t2; \
    maskpartialbits((x), (w), tmpmask); \
    t1 = SCRRIGHT((src), (x)); \
    t2 = DoRRop(rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
} \
else \
{ \
    int m; \
    int n; \
    int t1, t2; \
    m = 32-(x); \
    n = (w) - m; \
    t1 = SCRRIGHT((src), (x)); \
    t2 = DoRRop(rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & endtab[x]) | (t2 & starttab[x]); \
    t1 = SCRLEFT((src), m); \
    t2 = DoRRop(rop, t1, *((pdst) + 1)); \
    *((pdst)+1) = (*((pdst)+1) & starttab[n]) | (t2 & endtab[n]); \
}

#if GETLEFTBITS_ALIGNMENT == 1
#define getleftbits(psrc, w, dst)	getbits(psrc, 0, w, dst)
#endif /* GETLEFTBITS_ALIGNMENT == 1 */

#if GETLEFTBITS_ALIGNMENT == 2
#define getleftbits(psrc, w, dst) \
    { \
	if ( ((int)(psrc)) & 0x01 ) \
		getbits( ((unsigned int *)((int)(psrc))-1), 8, (w), (dst) ); \
	else
		getbits(psrc, 0, w, dst)
    }
#endif /* GETLEFTBITS_ALIGNMENT == 2 */

#if GETLEFTBITS_ALIGNMENT == 4
#define getleftbits(psrc, w, dst) \
    { \
	int off; \
	off = ( ((int)(psrc)) & 0x03) << 3; \
	getbits( \
		(unsigned int *)( ((int)(psrc)) &~0x03), \
		(off), (w), (dst) \
	       ); \
    }
#endif /* GETLEFTBITS_ALIGNMENT == 4 */


#define getshiftedleftbits(psrc, offset, w, dst) \
	getleftbits((psrc), (w), (dst)); \
	dst = SCRLEFT((dst), (offset));


