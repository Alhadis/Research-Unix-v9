/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/


#include "X.h"
#include "scrnintstr.h"
#include "colormap.h"
#include "colormapst.h"
#include "resource.h"

int
cfbListInstalledColormaps(pScreen, pmaps)
    ScreenPtr	pScreen;
    Colormap	*pmaps;
{
    *pmaps = pScreen->defColormap;
    return (1);
}

#ifdef	STATIC_COLOR
void
cfbResolveStaticColor(pred, pgreen, pblue, pVisual)
    unsigned short	*pred, *pgreen, *pblue;
    VisualPtr		pVisual;
{
    /* XXX - this works for the StaticColor visual ONLY */
    *pred &= 0xe000;
    *pgreen &= 0xe000;
    *pblue &= 0xc000;
}
#endif

ColormapPtr
cfbGetStaticColormap(pVisual)
    VisualPtr	pVisual;
{
    return (
	    (ColormapPtr)
		LookupID(screenInfo.screen[pVisual->screen].defColormap,
			 RT_COLORMAP, RC_CORE)
	    );
}

void
cfbInitialize332Colormap(pmap)
    ColormapPtr	pmap;
{
    int	i;

    for(i = 0; i < pmap->pVisual->ColormapEntries; i++)
    {
	/* XXX - assume 256 for now */
	pmap->red[i].co.local.red = (i & 0x7) << 13;
	pmap->red[i].co.local.green = (i & 0x38) << 10;
	pmap->red[i].co.local.blue = (i & 0xc0) << 8;
    }
}
