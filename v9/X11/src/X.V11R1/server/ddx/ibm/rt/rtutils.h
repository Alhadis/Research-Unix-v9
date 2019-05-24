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
#ifndef RT_UTILS
#define RT_UTILS 1

/* $Header: rtutils.h,v 5.1 87/09/13 03:31:43 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtutils.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidrtutils = "$Header: rtutils.h,v 5.1 87/09/13 03:31:43 erik Exp $";
#endif

extern	int		print_pattern();
extern	int		print_event();

extern	int		rtTrace;
#ifdef TRACE_X
#define	TRACE(f)	if (rtTrace) ErrorF f;
#else
#define TRACE(f)
#endif

#endif /* RT_UTILS */
