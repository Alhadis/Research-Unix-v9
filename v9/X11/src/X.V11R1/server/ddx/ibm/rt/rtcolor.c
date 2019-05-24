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

/* $Header: rtcolor.c,v 1.1 87/09/13 03:27:41 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtcolor.c,v $ */

#ifndef lint
static char *rcsid = "$Header: rtcolor.c,v 1.1 87/09/13 03:27:41 erik Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <machinecons/xio.h>

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"
#include "miscstruct.h"
#include "input.h"
#include "colormapst.h"
#include "resource.h"

#include "mfb.h"

#include "rtdecls.h"

void
rtResolveColorMono(pred, pgreen, pblue, pVisual)
    unsigned short	*pred, *pgreen, *pblue;
    VisualPtr		pVisual;
{
    TRACE(("rtResolveColorMono(pred=0x%x,pgreen=0x%x,pblue=0x%x,pVis=0x%x)\n",
						pred,pgreen,pblue,pVisual));
    /* Gets intensity from RGB.  If intensity is >= half, pick white, else
     * pick black.  This may well be more trouble than it's worth. */
    *pred = *pgreen = *pblue = 
        (((39L * *pred +
           50L * *pgreen +
           11L * *pblue) >> 8) >= (((1<<8)-1)*50)) ? ~0 : 0;
}

void
rtCreateColormapMono(pmap)
    ColormapPtr	pmap;
{
    int	red, green, blue, pix;

    TRACE(("rtCreateColormapMono(pmap=0x%x)\n",pmap));
    /* this is a monochrome colormap, it only has two entries, just fill
     * them in by hand.  If it were a more complex static map, it would be
     * worth writing a for loop or three to initialize it */
    red = green = blue = 0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);
    red = green = blue = ~0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);
}

void
rtDestroyColormapMono(pmap)
    ColormapPtr	pmap;
{
    TRACE(("rtDestroyColormapMono(pmap=0x%x)\n",pmap));
}

