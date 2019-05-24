/*
 *	rcs_id[] = "$Header: xmh.h,v 1.12 87/09/11 08:18:39 toddb Exp $";
 */

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

#ifndef _xmh_h
#define _xmh_h
#include <stdio.h>
#include <strings.h>
#include <Xlib.h>

#ifdef X11
#include <Xutil.h>
#include <cursorfont.h>
#endif

#include <Intrinsic.h>
#include <Atoms.h>
#include <ButtonBox.h>
#include <Command.h>
#include <Dialog.h>
#include <Form.h>
#include <Label.h>
#include <Scroll.h>
#include <Text.h>
#include <VPane.h>
#include <TextDisp.h>

#define DELETEABORTED	-1
#define MARKPOS		4

#define xMargin 2
#define yMargin 2

typedef int * dp;		/* For debugging. */

typedef struct _ButtonRec *Button;
typedef struct _ButtonBoxRec *ButtonBox;
typedef struct _TocRec *Toc;
typedef struct _MsgRec *Msg;
typedef struct _PickRec *Pick;

typedef enum {
    Fignore, Fmove, Fcopy, Fdelete
} FateType;

typedef enum {
    STtocAndView,
    STview,
    STcomp,
    STpick
} ScrnKind;

typedef struct _ScrnRec {
   Window	window;		/* Window containing the scrn */
   int		mapped;		/* TRUE only if we've mapped this screen. */
   ScrnKind	kind;		/* What kind of scrn we have. */
   ButtonBox	folderbuttons;	/* Folder buttons. */
   Button	curfolder;	/* Which is the current folder. */
   ButtonBox	mainbuttons;	/* Main xmh control buttons. */
   Window	toclabel;	/* Toc titlebar. */
   Window	tocwindow;	/* Toc text. */
   ButtonBox	tocbuttons;	/* Toc control buttons. */
   ButtonBox 	seqbuttons;	/* Sequence buttons. */
   Button	curseq;		/* Which is the current sequence. */
   Window	viewlabel;	/* View titlebar. */
   Window	viewwindow;	/* View window. */
   ButtonBox 	viewbuttons;	/* View control buttons. */
   Toc		toc;		/* The table of contents. */
   XtTextSink	*tocsink;	/* Sink used to display the toc. */
   Msg		msg;		/* The message being viewed. */
   Pick		pick;		/* Pick in this screen. */
#ifdef X11
   XWMHints	hints;		/* Record of hints to window manager. */
#endif X11
} ScrnRec, *Scrn;


typedef struct {
    int nummsgs;
    Msg *msglist;
} MsgListRec, *MsgList;


typedef struct {
   char		*name;		/* Name of this sequence. */
   MsgList	mlist;		/* Messages in this sequence. */
} SequenceRec, *Sequence;


#include "globals.h"
#include "macros.h"
#include "externs.h"
#include "mlist.h"
#include "bbox.h"
#include "msg.h"
#include "toc.h"

#endif _xmh_h
