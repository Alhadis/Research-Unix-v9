/* $Header: bboxint.h,v 1.1 87/09/11 08:19:11 toddb Exp $ */
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

/* Includes for modules implementing buttonbox stuff. */

#ifndef _bboxint_h
#define _bboxint_h

typedef struct _ButtonRec {
    Window	window;		/* Window containing this button. */
    ButtonBox	buttonbox;	/* Button box containing this button. */
    char	*name;	/* Name of the button. */
    short	enabled;	/* Whether this button is enabled
				   right now or not. */
    void	(*func)();	/* Function to be called when this 
				   button is pressed. */
    short	needsadding;	/* TRUE if we still need to actually go add
				   this button to the buttonbox. */
} ButtonRec;

typedef struct _ButtonBoxRec {
    Window	outer;		/* Window containing scollbars & inner */
    Window	inner;		/* Window containing the buttons. */
    Scrn	scrn;		/* Scrn containing this button box. */
    int		numbuttons;	/* How many buttons in this box. */
    Button	*button;	/* Array of pointers to buttons. */
    int		maxheight;	/* Current maximum height. */
    int		needsadding;	/* There are buttons that need to be added. */
    int		updatemode;	/* TRUE if refreshing; FALSE if refreshing is
				   currently inhibited. */
    Button	*radio;		/* Pointer to where to keep which radio button
				   is selected. */
    short	fullsized;	/* TRUE if we keep this box full-height. */
} ButtonBoxRec;

#endif _bboxint_h
