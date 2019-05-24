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
/* $Header: mfbcmap.c,v 1.13 87/09/04 14:19:49 toddb Exp $ */
#include "X.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"

extern int	TellLostMap(), TellGainedMap();
/* A monochrome frame buffer is a static gray colormap with two entries.
 * We have a "required list" of length 1.  Because we can only support 1
 * colormap, we never have to change it, but we may have to change the 
 * name we call it.  If someone installs a new colormap, we know it must
 * look just like the old one (because we've checked in dispatch that it was
 * a valid colormap identifier, and all the colormap IDs for this device
 * look the same).  Nevertheless, we still have to uninstall the old colormap
 * and install the new one.  Similarly, if someone uninstalls a colormap,
 * we have to install the default map, even though we know those two looked
 * alike.  
 * The required list concept is pretty much irrelevant when you can only
 * have one map installed at a time.  
 */
static ColormapPtr pInstalledMap = (ColormapPtr) None;
int
mfbListInstalledColormaps(pScreen, pmaps)
    ScreenPtr	pScreen;
    Colormap	*pmaps;
{
    /* By the time we are processing requests, we can guarantee that there
     * is always a colormap installed */
    *pmaps = pInstalledMap->mid;
    return (1);
}


void
mfbInstallColormap(pmap)
    ColormapPtr	pmap;
{
    if(pmap != pInstalledMap)
    {
	/* Uninstall pInstalledMap. No hardware changes required, just
	 * notify all interested parties. */
	if(pInstalledMap != (ColormapPtr)None)
	    WalkTree(pmap->pScreen, TellLostMap, &pInstalledMap->mid);
	/* Install pmap */
	pInstalledMap = pmap;
	WalkTree(pmap->pScreen, TellGainedMap, &pmap->mid);

    }
}
void
mfbUninstallColormap(pmap)
    ColormapPtr	pmap;
{
    if(pmap == pInstalledMap)
    {
        /* Uninstall pmap */
	WalkTree(pmap->pScreen, TellLostMap, &pmap->mid);
	/* Install default map */
	pInstalledMap = (ColormapPtr) LookupID(pmap->pScreen->defColormap,
					       RT_COLORMAP, RC_CORE);
	WalkTree(pmap->pScreen, TellGainedMap, &pInstalledMap->mid);
    }
	
}
