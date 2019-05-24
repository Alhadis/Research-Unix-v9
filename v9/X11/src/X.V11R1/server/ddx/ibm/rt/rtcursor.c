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
/* $Header: rtcursor.c,v 1.5 87/09/13 03:28:52 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtcursor.c,v $ */

#ifndef lint
static char *rcsid = "$Header: rtcursor.c,v 1.5 87/09/13 03:28:52 erik Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <machinecons/xio.h>

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "input.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "miscstruct.h"

#include "rtio.h"
#include "rtcursor.h"
#include "rtutils.h"

/***============================================================***/

int	rtSoftCursor=	 FALSE;
short	*rtCursorX=	 NULL;
short	*rtCursorY=	 NULL;
short	rtCursorHotX=		 0;
short	rtCursorHotY=		 0;
BoxRec	rtCursorBounds;
int	(*rtCursorShow)()=	(int (*)())NULL;

/***============================================================***/

    /*
     * munge the SERVER-DEPENDENT, device-independent cursor bits into
     * what the device wants.
     * Server cursor image is *fullword* aligned,
     * RT cursor image is 16x16.
     */

#define SERVER_PAD(w)	((((w)+31)/32)*4)

Bool
rtRealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr 	pCurs;	/* a SERVER-DEPENDENT cursor */
{
    QIOLocator	*loc;
    int		row_bytes;
    int		i,num_rows;

    TRACE(("rtRealizeCursor( pScr= 0x%x, pCurs= 0x%x)\n",pScr,pCurs));

    loc = (QIOLocator *)Xalloc( sizeof(QIOLocator) );
    bzero( (char *)loc, sizeof(QIOLocator) );
    pCurs->devPriv[ pScr->myNum ]= (pointer)loc;
    row_bytes= SERVER_PAD(pCurs->width);
    num_rows= (16<pCurs->height?16:pCurs->height);

    for (i=0;i<num_rows;i++) {
       loc->data[i]= ((~pCurs->source[row_bytes*i]
	      & pCurs->mask[row_bytes*i])<<8)&0xff00;
       loc->mask[i]= (pCurs->mask[row_bytes*i]<<8)&0xff00;
       if (pCurs->width>8) {	/* second byte? */
	  loc->data[i]|= ~pCurs->source[row_bytes*i+1]
	      & pCurs->mask[row_bytes*i+1];
	  loc->mask[i]|= pCurs->mask[row_bytes*i+1];
       }
    }
    loc->hotSpot.v=	pCurs->yhot;
    loc->hotSpot.h=	pCurs->xhot;
    return(TRUE);
}

/***============================================================***/

Bool
rtUnrealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    TRACE(("rtUnrealizeCursor( pScr= 0x%x, pCurs= 0x%x )\n",pScr,pCurs));

    Xfree( pCurs->devPriv[ pScr->myNum]);
    return(TRUE);
}

/***============================================================***/

int
rtDisplayCursor( pScr, pCurs )
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    TRACE(("rtDisplayCursor( pScr= 0x%x, pCurs= 0x%x )\n",pScr,pCurs));

    rtSoftCursor= FALSE;
    ioctl(rtScreenFD, QIOCLDCUR, pCurs->devPriv[ pScr->myNum ]);
}

/***============================================================***/

int
rtSetCursorPosition( pScr, x, y, generateEvent )
    ScreenPtr 	pScr;
    int		x,y;
    Bool	generateEvent;
{
    XCursor	pos;
    xEvent	motion;
    DevicePtr	mouse;

    TRACE(("rtSetCursorPosition( pScr= 0x%x, x= %d, y= %d )\n",pScr,x,y));

    pos.x= x;
    pos.y= y;
    if (rtSoftCursor) {
	if ((x<rtCursorBounds.x1)||(x>rtCursorBounds.x2)||
	    (y<rtCursorBounds.y1)||(y>rtCursorBounds.y2)) {
		(*rtCursorShow)(&pos.x,&pos.y);
	}
	else {
		(*rtCursorX)= x-rtCursorHotX;
		(*rtCursorY)= y-rtCursorHotY;
	}
    }
    if (generateEvent)
    {
	if (rtQueue->head != rtQueue->tail)
	    ProcessInputEvents();
	motion.u.keyButtonPointer.rootX = x;
	motion.u.keyButtonPointer.rootY = y;
	motion.u.keyButtonPointer.time = lastEventTime;
	motion.u.u.type = MotionNotify;

	mouse = LookupPointerDevice();
	(*mouse->processInputProc) (&motion, mouse);
    }
    return(ioctl(rtScreenFD, QIOCSMSTATE, (caddr_t) &pos));
}

/***============================================================***/

void
rtPointerNonInterestBox( pScr, pBox )
ScreenPtr	pScr;
BoxPtr		pBox;
{

    TRACE(("rtPointerNonInterestBox( pScr= 0x%x, pBox= 0x%x )\n"));

    rtXaddr->mbox.top=		pBox->y1;
    rtXaddr->mbox.bottom=	pBox->y2;
    rtXaddr->mbox.left=		pBox->x1;
    rtXaddr->mbox.right=	pBox->x2;
}

/***============================================================***/

void
rtConstrainCursor( pScr, pBox )
ScreenPtr	pScr;
BoxPtr		pBox;
{
    TRACE(("rtConstrainCursor( pScr= 0x%x, pBox= 0x%x )\n"));
}

/***============================================================***/

void
rtCursorLimits( pScr, pCurs, pHotBox, pTopLeftBox )
ScreenPtr	pScr;
CursorPtr	pCurs;
BoxPtr		pHotBox;
BoxPtr		pTopLeftBox;
{
    TRACE(("rtCursorLimits( pScr= 0x%x, pCurs= 0x%x, pHotBox= 0x%x, pTopLeftBox= 0x%x)\n", pScr, pCurs, pHotBox, pTopLeftBox));

    pTopLeftBox->x1= max( pHotBox->x1, 0 );
    pTopLeftBox->y1= max( pHotBox->y1, 0 );
    pTopLeftBox->x2= min( pHotBox->x2, pScr->width );
    pTopLeftBox->y2= min( pHotBox->y2, pScr->height );
}

