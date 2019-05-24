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
/* $Header: mfbpixmap.c,v 1.45 87/09/07 19:03:20 rws Exp $ */

/* pixmap management
   written by drewry, september 1986

   on a monchrome device, a pixmap is a bitmap.
*/

#include "Xmd.h"
#include "pixmapstr.h"
#include "maskbits.h"

#include "mfb.h"
#include "mi.h"

#include "servermd.h"

PixmapPtr
mfbCreatePixmap (pScreen, width, height, depth)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
{
    register PixmapPtr pPixmap;
    int size;

    if (depth != 1)
	return (PixmapPtr)NULL;

    pPixmap = (PixmapPtr)Xalloc(sizeof(PixmapRec));
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.depth = 1;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->width = width;
    pPixmap->height = height;
    pPixmap->devKind = PixmapBytePad(width, 1);
    pPixmap->refcnt = 1;
    size = height * pPixmap->devKind;

    if ( !(pPixmap->devPrivate = (pointer)Xalloc(size)))
    {
	Xfree(pPixmap);
	return (PixmapPtr)NULL;
    }
    else
        bzero((char *)pPixmap->devPrivate, size);
    return pPixmap;
}

Bool
mfbDestroyPixmap(pPixmap)
    PixmapPtr pPixmap;
{
/* BOGOSITY ALERT */
    if ((unsigned)pPixmap < 42)
	return TRUE;

    if(--pPixmap->refcnt)
	return TRUE;
    Xfree(pPixmap->devPrivate);
    Xfree(pPixmap);
    return TRUE;
}


PixmapPtr
mfbCopyPixmap(pSrc)
    register PixmapPtr	pSrc;
{
    register PixmapPtr	pDst;
    register int	*pDstPriv, *pSrcPriv, *pDstMax;
    int		size;

    pDst = (PixmapPtr) Xalloc(sizeof(PixmapRec));
    pDst->drawable.type = pSrc->drawable.type;
    pDst->drawable.pScreen = pSrc->drawable.pScreen;
    pDst->width = pSrc->width;
    pDst->height = pSrc->height;
    pDst->drawable.depth = pSrc->drawable.depth;
    pDst->devKind = pSrc->devKind;
    pDst->refcnt = 1;

    size = pDst->height * pDst->devKind;
    pDstPriv = (int *) Xalloc(size);
    pDst->devPrivate = (pointer) pDstPriv;
    if (!(pDstPriv))
    {
	Xfree(pDst);
	return NullPixmap;
    }
    else
	bzero((char *) pDstPriv, size);
    pSrcPriv = (int *)pSrc->devPrivate;
    pDstMax = pDstPriv + (size >> 2);
    /* Copy words */
    while(pDstPriv < pDstMax)
    {
        *pDstPriv++ = *pSrcPriv++;
    }

    return pDst;
}


/* replicates a pattern to be a full 32 bits wide.
   relies on the fact that each scnaline is longword padded.
   doesn't do anything if pixmap is not a factor of 32 wide.
   changes width field of pixmap if successful, so that the fast
	XRotatePixmap code gets used if we rotate the pixmap later.

   calculate number of times to repeat
   for each scanline of pattern
      zero out area to be filled with replicate
      left shift and or in original as many times as needed

   returns TRUE iff pixmap was, or could be padded to be, 32 bits wide.
*/
Bool
mfbPadPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    register int width = pPixmap->width;
    register int h;
    register int mask;
    register unsigned int *p;
    register unsigned int bits;	/* real pattern bits */
    register int i;
    int rep;			/* repeat count for pattern */

    if (width == 32)
	return(TRUE);
    if (width > 32)
	return(FALSE);

    rep = 32/width;
    if (rep*width != 32)
	return(FALSE);

    mask = endtab[width];

    p = (unsigned int *)(pPixmap->devPrivate);
    for (h=0; h < pPixmap->height; h++)
    {
	*p &= mask;
	bits = *p;
	for(i=1; i<rep; i++)
	{
	    bits = SCRRIGHT(bits, width);
	    *p |= bits;
	}
	p++;
    }
    pPixmap->width = 32;
    return(TRUE);
}

/* Rotates pixmap pPix by w pixels to the right on the screen. Assumes that
 * words are 32 bits wide, and that the least significant bit appears on the
 * left.
 */
mfbXRotatePixmap(pPix, rw)
    PixmapPtr	pPix;
    register int rw;
{
    register long	*pw, *pwFinal;
    register unsigned long	t;

    if (pPix == NullPixmap)
        return;

    pw = (long *)pPix->devPrivate;
    rw %= pPix->width;
    if (rw < 0)
	rw += pPix->width;
    if(pPix->width == 32)
    {
        pwFinal = pw + pPix->height;
	while(pw < pwFinal)
	{
	    t = *pw;
	    *pw++ = SCRRIGHT(t, rw) | 
		    (SCRLEFT(t, (32-rw)) & endtab[rw]);
	}
    }
    else
    {
	/* We no longer do this.  Validate doesn't try to rotate odd-size
	 * tiles or stipples.  mfbUnnatural<tile/stipple>FS works directly off
	 * the unrotate tile/stipple in the GC
	 */
        ErrorF("X internal error: trying to rotate odd-sized pixmap.\n");
    }

}
/* Rotates pixmap pPix by h lines.  Assumes that h is always less than
   pPix->height
   works on any width.
 */
mfbYRotatePixmap(pPix, rh)
    register PixmapPtr	pPix;
    int	rh;
{
    int nbyDown;	/* bytes to move down to row 0; also offset of
			   row rh */
    int nbyUp;		/* bytes to move up to line rh; also
			   offset of first line moved down to 0 */
    char *pbase;
    char *ptmp;

    if (pPix == NullPixmap)
	return;
    rh %= pPix->height;
    if (rh < 0)
	rh += pPix->height;

    pbase = (char *)pPix->devPrivate;

    nbyDown = rh * pPix->devKind;
    nbyUp = (pPix->devKind * pPix->height) - nbyDown;
    if(!(ptmp = (char *)ALLOCATE_LOCAL(nbyUp)))
	return;

    bcopy(pbase, ptmp, nbyUp);		/* save the low rows */
    bcopy(pbase+nbyUp, pbase, nbyDown);	/* slide the top rows down */
    bcopy(ptmp, pbase+nbyDown, nbyUp);	/* move lower rows up to row rh */
    DEALLOCATE_LOCAL(ptmp);
}

