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
#ifndef RT_INIT
#define RT_INIT 1

/* $Header: rtinit.h,v 5.1 87/09/13 03:29:44 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtinit.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidrtinit = "$Header: rtinit.h,v 5.1 87/09/13 03:29:44 erik Exp $";
#endif

#define	NUMSCREENS	1
extern	Bool	(*screenInitProcs[NUMSCREENS])();
extern	Bool	(*screenCloseProcs[NUMSCREENS])();
extern	int	InitOutput();
extern	int	InitInput();
extern	int	rtProcessCommandLine();

#endif /* RT_INIT */
