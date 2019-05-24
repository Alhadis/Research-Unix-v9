#ifndef lint
static char rcs_id[] = "$Header: icon.c,v 1.3 87/09/12 08:03:02 swick Exp $";
#endif lint
/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documentation,
 * and that the name of Digital Equipment Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 */

/* Icon.c - Handle icon pixmaps. */

#include "xmh.h"
#include "nomail.bit"
#include "newmail.bit"


void IconInit()
{
#ifdef X11
    NoMailPixmap = XCreateBitmapFromData( theDisplay,
					  DefaultRootWindow(theDisplay),
					  nomail_bits,
					  nomail_width, nomail_height );

    NewMailPixmap = XCreateBitmapFromData( theDisplay,
					   DefaultRootWindow(theDisplay),
					   newmail_bits,
					   newmail_width, newmail_height);
#endif
}
