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
/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* $Header: apa16pntw.c,v 5.3 87/09/13 03:20:53 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16pntw.c,v $ */

#ifndef lint
static char *rcsid = "$Header: apa16pntw.c,v 5.3 87/09/13 03:20:53 erik Exp $";
#endif

#include "X.h"
#include "Xmd.h"

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"

#include "rtutils.h"

#include "apa16hdwr.h"

void
apa16PaintWindowSolid(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    int nbox;		/* number of boxes to fill */
    register BoxPtr pbox;	/* pointer to list of boxes to fill */
    int	     rop;
    unsigned	cmd;
    int		w,h;

    TRACE(("apa16PaintWindowSolid( pWin= 0x%x, pRegion= 0x%x, what= %d )\n",
							pWin,pRegion,what));

    nbox = pRegion->numRects;
    pbox = pRegion->rects;

    if (!nbox)	return;

    if (what == PW_BACKGROUND)
    {
        rop= pWin->backgroundPixel ? RROP_WHITE: RROP_BLACK;
    } 
    else
    {
        rop = pWin->borderPixel ? RROP_WHITE : RROP_BLACK;
    } 

    QUEUE_RESET();
    APA16_GET_CMD(ROP_RECT_FILL,rop,cmd);

    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

	FILL_RECT(cmd,pbox->x2,pbox->y2,w,h);
        pbox++;
    }
    APA16_GO();
}

