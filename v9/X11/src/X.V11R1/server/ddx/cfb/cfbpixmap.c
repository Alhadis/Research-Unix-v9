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
/* pixmap management
   written by drewry, september 1986

   on a monchrome device, a pixmap is a bitmap.
*/

#include "Xmd.h"
#include "servermd.h"
#include "pixmapstr.h"
#include "cfbmskbits.h"

#include "cfb.h"
#include "mi.h"

PixmapPtr
cfbCreatePixmap (pScreen, width, height, depth, format)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
    int		format;
{
    register PixmapPtr pPixmap;
    int size;

    if (depth != 1 && depth != PSZ)
	return (PixmapPtr)NULL;

    pPixmap = (PixmapPtr)Xalloc(sizeof(PixmapRec));
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->width = width;
    pPixmap->height = height;
    pPixmap->devKind = PixmapBytePad(width, depth);
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
cfbDestroyPixmap(pPixmap)
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
cfbCopyPixmap(pSrc)
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
	cfbXRotatePixmap code gets used if we rotate the pixmap later.
	cfbXRotatePixmap code gets used if we rotate the pixmap later.

   calculate number of times to repeat
   for each scanline of pattern
      zero out area to be filled with replicate
      left shift and or in original as many times as needed

   returns TRUE iff pixmap was, or could be padded to be, 32 bits wide.
*/
Bool
cfbPadPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    /* do nothing for now; eventually pad to a 4-byte boundary. */
    return( FALSE );
}


#ifdef notdef
/*
 * cfb debugging routine -- assumes pixmap is 1 byte deep 
 */
static cfbdumppixmap(pPix)
    PixmapPtr	pPix;
{
    unsigned int *pw;
    char *psrc, *pdst;
    int	i, j;
    char	line[66];

    ErrorF(  "pPixmap: 0x%x\n", pPix);
    ErrorF(  "%d wide %d high\n", pPix->width, pPix->height);
    if (pPix->width > 64)
    {
	ErrorF(  "too wide to see\n");
	return;
    }

    pw = (unsigned int *) pPix->devPrivate;
    psrc = (char *) pw;

/*
    for ( i=0; i<pPix->height; ++i )
	ErrorF( "0x%x\n", pw[i] );
*/

    for ( i = 0; i < pPix->height; ++i ) {
	pdst = line;
	for(j = 0; j < pPix->width; j++) {
	    *pdst++ = *psrc++ ? 'X' : ' ' ;
	}
	*pdst++ = '\n';
	*pdst++ = '\0';
	ErrorF( "%s", line);
    }
}
#endif notdef

/* Rotates pixmap pPix by w pixels to the right on the screen. Assumes that
 * words are 32 bits wide, and that the least significant bit appears on the
 * left.
 */
cfbXRotatePixmap(pPix, rw)
    PixmapPtr	pPix;
    register int rw;
{
    register long	*pw, *pwFinal, *pwNew;
    register unsigned long	t;
    int			size;

    if (pPix == NullPixmap)
        return;

    switch (((DrawablePtr) pPix)->depth) {
	case PSZ:
	    break;
	case 1:
	    cfbXRotateBitmap(pPix, rw);
	    return;
	default:
	    ErrorF("cfbXRotatePixmap: unsupported depth %d\n", ((DrawablePtr) pPix)->depth);
	    return;
    }
    pw = (long *)pPix->devPrivate;
    rw %= pPix->width;
    if (rw < 0)
	rw += pPix->width;
    if(pPix->width == PPW)
    {
        pwFinal = pw + pPix->height;
	while(pw < pwFinal)
	{
	    t = *pw;
	    *pw++ = SCRRIGHT(t, rw) | 
		    (SCRLEFT(t, (PPW-rw)) & cfbendtab[rw]);
	}
    }
    else
    {
	pwNew = (long *) Xalloc( pPix->height * pPix->devKind);

	/* o.k., divide pw (the pixmap) in two vertically at (w - rw)
	 * pick up the part on the left and make it the right of the new
	 * pixmap.  then pick up the part on the right and make it the left
	 * of the new pixmap.
	 * now hook in the new part and throw away the old. All done.
	 */
	size = PixmapBytePad(pPix->width, PSZ) >> 2;
        cfbQuickBlt(pw, pwNew, 0, 0, rw, 0, pPix->width - rw, pPix->height,
		    size, size);
        cfbQuickBlt(pw, pwNew, pPix->width - rw, 0, 0, 0, rw, pPix->height,
	            size, size);
	pPix->devPrivate = (pointer) pwNew;
	Xfree((char *) pw);

    }

}
/* Rotates pixmap pPix by h lines.  Assumes that h is always less than
   pPix->height
   works on any width.
 */
cfbYRotatePixmap(pPix, rh)
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
    switch (((DrawablePtr) pPix)->depth) {
	case PSZ:
	    break;
	case 1:
	    mfbYRotatePixmap(pPix, rh);
	    return;
	default:
	    ErrorF("cfbYRotatePixmap: unsupported depth %d\n", ((DrawablePtr) pPix)->depth);
	    return;
    }

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

