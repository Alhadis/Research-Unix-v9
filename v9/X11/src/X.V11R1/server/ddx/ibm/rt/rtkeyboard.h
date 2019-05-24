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
#ifndef RT_KEYBOARD
#define RT_KEYBOARD 1

/* $Header: rtkeyboard.h,v 5.2 87/09/13 03:30:27 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtkeyboard.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidrtkeyboard = "$Header: rtkeyboard.h,v 5.2 87/09/13 03:30:27 erik Exp $";
#endif

#define	RT_LED_NUMLOCK		1
#define RT_LED_CAPSLOCK		2
#define RT_LED_SCROLLOCK	4

extern	DevicePtr	rtKeybd;
extern	KeySym		rtmap[];
extern	int		rtKeybdProc();
extern	int		rtChangeLEDs();
extern	void		rtUsePCKeyboard();
extern	void		rtUseRTKeyboard();

#endif /* RT_KEYBOARD */
