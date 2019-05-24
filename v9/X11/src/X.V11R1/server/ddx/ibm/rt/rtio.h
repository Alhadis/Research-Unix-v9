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
#ifndef RT_IO
#define RT_IO 1

/* $Header: rtio.h,v 5.2 87/09/13 03:30:03 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtio.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidrtio = "$Header: rtio.h,v 5.2 87/09/13 03:30:03 erik Exp $";
#endif

XIoAddr		*rtXaddr;	
XEventQueue	*rtQueue;
extern	int	rtScreenFD;
extern	void	ProcessInputEvents();
extern	int	TimeSinceLastInputEvent();
extern	void	rtQueryBestSize();
extern	int	lastEventTime;

#endif /* RT_IO */
