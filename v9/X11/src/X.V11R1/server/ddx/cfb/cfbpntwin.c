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

#include "X.h"

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "cfbmskbits.h"
/*
NOTE
   PaintArea32() doesn't need to rotate the tile, since
cfbPositionWIndow() and cfbChangeWIndowAttributes() do it;
cfbPaintAreaOther(), however, needs to rotate things.
*/

/* Paint Area None -- just return */
void
cfbPaintAreaNone(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    if ( pWin->drawable.depth != PSZ )
	FatalError( "cfbPaintAreaNone: invalid depth\n" );
}

/* Paint Area Parent Relative -- Find first ancestor which isn't parent
 * relative and paint as it would, but with this region */ 
void
cfbPaintAreaPR(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    WindowPtr pParent;

    if ( pWin->drawable.depth != PSZ )
	FatalError( "cfbPaintAreaPR: invalid depth\n" );

    pParent = pWin->parent;
    while(pParent->backgroundTile == (PixmapPtr)ParentRelative)
	pParent = pParent->parent;

    if(what == PW_BORDER)
        (*pParent->PaintWindowBorder)(pParent, pRegion, what);
    else
	(*pParent->PaintWindowBackground)(pParent, pRegion, what);
}

void
cfbPaintAreaSolid(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    int nbox;		/* number of boxes to fill */
    register BoxPtr pbox;	/* pointer to list of boxes to fill */
    register int srcpix;/* source pixel of the window */

    PixmapPtr pPixmap;
    int nlwScreen;	/* width in longwords of the screen's pixmap */
    int w;		/* width of current box */
    register int h;	/* height of current box */
    int startmask;
    int endmask;	/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    register int nlwExtra;	
		        /* to get from right of box to left of next span */
    register int nlw;	/* loop version of nlwMiddle */
    register int *p;	/* pointer to bits we're writing */
    int *pbits;		/* pointer to start of screen */

    if ( pWin->drawable.depth != PSZ )
	FatalError( "cfbPaintAreaSolid: invalid depth\n" );

    if (what == PW_BACKGROUND)
    {
        srcpix = PFILL(pWin->backgroundPixel);
    } 
    else
    {
        srcpix = PFILL(pWin->borderPixel);
    } 

    pPixmap = (PixmapPtr)(pWin->drawable.pScreen->devPrivate);
    pbits = (int *)pPixmap->devPrivate;
    nlwScreen = (pPixmap->devKind) >> 2;
    nbox = pRegion->numRects;
    pbox = pRegion->rects;

    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	p = pbits + (pbox->y1 * nlwScreen) + (pbox->x1 >> PWSH);

	if ( ((pbox->x1 & PIM) + w) < PPW)
	{
	    maskpartialbits(pbox->x1, w, startmask);
	    nlwExtra = nlwScreen;
	    while (h--)
	    {
		*p = (*p & ~startmask) | (srcpix & startmask);
		p += nlwExtra;
	    }
	}
	else
	{
	    maskbits(pbox->x1, w, startmask, endmask, nlwMiddle);
	    nlwExtra = nlwScreen - nlwMiddle;

	    if (startmask && endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    while (nlw--)
			*p++ = srcpix;
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else if (startmask && !endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    while (nlw--)
			*p++ = srcpix;
		    p += nlwExtra;
		}
	    }
	    else if (!startmask && endmask)
	    {
		while (h--)
		{
		    nlw = nlwMiddle;
		    while (nlw--)
			*p++ = srcpix;
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else /* no ragged bits at either end */
	    {
		while (h--)
		{
		    nlw = nlwMiddle;
		    while (nlw--)
			*p++ = srcpix;
		    p += nlwExtra;
		}
	    }
	}
        pbox++;
    }
}

/* Tile area with a 32 bit wide tile */
void
cfbPaintArea32(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    int nbox;		/* number of boxes to fill */
    register BoxPtr pbox;	/* pointer to list of boxes to fill */
    int srcpix;	
    int *psrc;		/* pointer to bits in tile, if needed */
    int tileHeight;	/* height of the tile */

    PixmapPtr pPixmap;
    int nlwScreen;	/* width in longwords of the screen's pixmap */
    int w;		/* width of current box */
    register int h;	/* height of current box */
    int startmask;
    int endmask;	/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    register int nlwExtra;	
		        /* to get from right of box to left of next span */
    
    register int nlw;	/* loop version of nlwMiddle */
    register int *p;	/* pointer to bits we're writing */
    int y;		/* current scan line */


    int *pbits;		/* pointer to start of screen */
    cfbPrivWin *pPrivWin;

    if ( pWin->drawable.depth != PSZ )
	FatalError( "cfbPaintArea32: invalid depth\n" );

    pPrivWin = (cfbPrivWin *)(pWin->devPrivate);

    if (what == PW_BACKGROUND)
    {
	tileHeight = pWin->backgroundTile->height;
	psrc = (int *)(pPrivWin->pRotatedBackground->devPrivate);
    } 
    else
    {
        tileHeight = pWin->borderTile->height;
	psrc = (int *)(pPrivWin->pRotatedBorder->devPrivate);
    } 

    pPixmap = (PixmapPtr)(pWin->drawable.pScreen->devPrivate);
    pbits = (int *)pPixmap->devPrivate;
    nlwScreen = (pPixmap->devKind) >> 2;
    nbox = pRegion->numRects;
    pbox = pRegion->rects;

    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	y = pbox->y1;
	p = pbits + (pbox->y1 * nlwScreen) + (pbox->x1 >> PWSH);

	if ( ((pbox->x1 & PIM) + w) < PPW)
	{
	    maskpartialbits(pbox->x1, w, startmask);
	    nlwExtra = nlwScreen;
	    while (h--)
	    {
		if (tileHeight)
		{
		    srcpix = psrc[y%tileHeight];
		    y++;
		}
		*p = (*p & ~startmask) | (srcpix & startmask);
		p += nlwExtra;
	    }
	}
	else
	{
	    maskbits(pbox->x1, w, startmask, endmask, nlwMiddle);
	    nlwExtra = nlwScreen - nlwMiddle;

	    if (startmask && endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    if (tileHeight)
		    {
			srcpix = psrc[y%tileHeight];
			y++;
		    }
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    while (nlw--)
			*p++ = srcpix;
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else if (startmask && !endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    if (tileHeight)
		    {
			srcpix = psrc[y%tileHeight];
			y++;
		    }
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    while (nlw--)
			*p++ = srcpix;
		    p += nlwExtra;
		}
	    }
	    else if (!startmask && endmask)
	    {
		while (h--)
		{
		    if (tileHeight)
		    {
			srcpix = psrc[y%tileHeight];
			y++;
		    }
		    nlw = nlwMiddle;
		    while (nlw--)
			*p++ = srcpix;
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else /* no ragged bits at either end */
	    {
		while (h--)
		{
		    if (tileHeight)
		    {
			srcpix = psrc[y%tileHeight];
			y++;
		    }
		    nlw = nlwMiddle;
		    while (nlw--)
			*p++ = srcpix;
		    p += nlwExtra;
		}
	    }
	}
        pbox++;
    }
}

void
cfbPaintAreaOther(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;	

{
    int nbox;		/* number of boxes to fill */
    register BoxPtr pbox;	/* pointer to list of boxes to fill */
    int tileHeight;	/* height of the tile */
    int tileWidth;	/* width of the tile */

    PixmapPtr pPixmap;
    int w;		/* width of current box */
    register int h;	/* height of current box */
    int x, y;		/* current scan line */
    int	height, width;

    if ( pWin->drawable.depth != PSZ )
	FatalError( "cfbPaintAreaOther: invalid depth\n" );

    if (what == PW_BACKGROUND)
    {
	tileHeight = pWin->backgroundTile->height;
	tileWidth = pWin->backgroundTile->width;
	pPixmap = pWin->backgroundTile;
    }
    else
    {
	tileHeight = pWin->borderTile->height;
	tileWidth = pWin->borderTile->width;
	pPixmap = pWin->borderTile;
    } 

    nbox = pRegion->numRects;
    pbox = pRegion->rects;

    while (nbox--)
    {
	w = pbox->x2;
	h = pbox->y2;

	if (  w - pbox->x1 <= PPW)
	{
	    y = pbox->y1;
	    x = pbox->x1;
	    width = min(tileWidth, w - x);
	    while ((height = h - y) > 0)
	    {
		height = min(height, tileHeight);
		cfbTileOddWin(pPixmap, pWin, width, height, x, y);
		y += tileHeight;
	    }
	}
	else
	{
	    y = pbox->y1;
	    while((height = h - y) > 0)
	    {
		height = min(height, tileHeight);
		x = pbox->x1;
		while ((width = w - x) > 0)
		{
		    width = min(tileWidth, width);
		    cfbTileOddWin(pPixmap, pWin, width, height, x, y);
		    x += tileWidth;
		}
		y += tileHeight;
	    }
	}
        pbox++;
    }
}



cfbTileOddWin(pSrc, pDstWin, tileWidth, tileHeight, x, y)
    PixmapPtr	pSrc;		/* pointer to src tile */
    WindowPtr	pDstWin;	/* pointer to dest window */
    int	   	tileWidth;	/* width of tile */
    int		tileHeight;	/* height of tile */
    int 	x;		/* destination x */
    int 	y;		/* destination y */
    
{
    int 		*psrcLine, *pdstLine;
    register int	*pdst, *psrc;
    register int	nl;
    register int	tmpSrc;
    int			widthSrc, widthDst, nlMiddle, startmask, endmask;
    PixmapPtr		pDstPixmap;


    psrcLine = (int *)pSrc->devPrivate;

    pDstPixmap = (PixmapPtr)pDstWin->drawable.pScreen->devPrivate;
    widthDst = (int)pDstPixmap->devKind >> 2;
    pdstLine = (int *)pDstPixmap->devPrivate + (y * widthDst);
    widthSrc = (int)pSrc->devKind >> 2;

    if(tileWidth <= PPW)
    {
        int dstBit;

        psrc = psrcLine;
	pdst = pdstLine + (x / PPW);
	dstBit = x & PIM;

	while(tileHeight--)
	{
	    getbits(psrc, 0, tileWidth, tmpSrc);
/*XXX*/	    putbits(tmpSrc, dstBit, tileWidth, pdst, -1);
	    pdst += widthDst;
	    psrc += widthSrc;
	}

    }
    else
    {
	register int xoffSrc;	/* offset (>= 0, < 32) from which to
			         * fetch whole longwords fetched in src */
	int nstart;		/* number of ragged bits at start of dst */
	int nend;		/* number of regged bits at end of dst */
	int srcStartOver;	/* pulling nstart bits from src overflows
			         * into the next word? */

	maskbits(x, tileWidth, startmask, endmask, nlMiddle);
	if (startmask)
	    nstart = PPW - (x & PIM);
	else
	    nstart = 0;
	if (endmask)
	    nend = (x + tileWidth) & PIM;

	xoffSrc = nstart & PIM;
	srcStartOver = nstart > PLST;

	pdstLine += (x >> PWSH);

	while (tileHeight--)
	{
	    psrc = psrcLine;
	    pdst = pdstLine;

	    if (startmask)
	    {
		getbits(psrc, 0, nstart, tmpSrc);
/*XXX*/		putbits(tmpSrc, (x & PIM), nstart, pdst, -1);
		pdst++;
#ifdef	notdef
/* XXX - not sure if this is right or not DSHR */
		if (srcStartOver)
		    psrc++;
#endif
	    }

	    nl = nlMiddle;
	    while (nl--)
	    {
		getbits(psrc, xoffSrc, PPW, tmpSrc);
		*pdst++ = tmpSrc;
		psrc++;
	    }

	    if (endmask)
	    {
		getbits(psrc, xoffSrc, nend, tmpSrc);
/*XXX*/		putbits(tmpSrc, 0, nend, pdst, -1);
	    }

	    pdstLine += widthDst;
	    psrcLine += widthSrc;
	}
    }
}
