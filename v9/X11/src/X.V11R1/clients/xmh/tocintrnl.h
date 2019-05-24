/* $Header: tocintrnl.h,v 1.1 87/09/11 08:19:22 toddb Exp $ */
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

/* Includes for modules implementing toc stuff. */

#ifndef _tocinternal_h
#define _tocinternal_h

typedef enum {
    unknown, valid, invalid
} ValidType;

typedef struct _MsgRec {
    Toc		toc;		/* Which toc this message is in. */
    Toc		desttoc;	/* Folder to copy or move to (NULL if none) */
    FateType	fate;		/* What will be done to this message */
    XtTextPosition position;	/* Position in the scanfile for this msg. */
    int		length;		/* #/chars for this msg's entry in scanfile. */
    char	*buf;		/* The scanline for this message. */
    int		msgid;		/* Message id for this message. */
    short	changed;	/* True iff this entry needs to be saved. */
    short	visible;	/* Whether we should show this message. */
    short	temporary;	/* Whether we should delete this message when
				   it is no longer visible. */
    Scrn	scrn;		/* Scrn showing this message (if any) */
    XtTextSource *source;	/* Source (if any) containing this msg. */
    short	reapable;	/* True iff we don't need to keep this
				   composition around. */
    XtTextPosition startPos;	/* Where to start the insertion point. */
} MsgRec;

typedef struct _TocRec {
   Scrn	scrn;			/* Scrn containing this table of contents */
   char 	*foldername;	/* Folder name for this toc */
   char		*path;		/* Full path to folder's directory. */
   char		*scanfile;	/* Full path to file containing scan. */
   Msg		curmsg;		/* Current msgid for this toc. */
   int		nummsgs;	/* How many info entries we currently have. */
   Msg		*msgs;		/* Array of pointers to info about each msg. */
   int		numsequences;	/* #/sequences defined for this folder. */
   Sequence *seqlist;	/* Array of pointers to sequences. */
   Sequence viewedseq;	/* Seq currently shown (NULL == all msgs) */
   XtTextSource	*source;	/* Source for the file containing info. */
   int		length;		/* #/chars in the scanfile. */
   int		origlength;	/* Original #/chars in the scanfile. */
   int		lastPos;	/* Last legal position */
   ValidType	validity;	/* Whether the scan file for this toc is */
				/* up to date. */
   int		needsrepaint;	/* TRUE if we should repaint this toc. */
   int		needscachesave;	/* TRUE if the cache needs saving. */
   int		needslabelupdate;/* TRUE if the toclabel needs repainting. */
   int		stopupdate;	/* Zero if refreshing; nonzero if refreshing is
				   currently inhibited. */
   char		*incfile;	/* Which file to incorporate from (if any). */
   int		mailpending;	/* True if we're currently displaying
				   mail-pending info for this folder. */
   long		lastreaddate;	/* Last time we read or wrote the cache. */
} TocRec;

#endif _tocinternal_h
