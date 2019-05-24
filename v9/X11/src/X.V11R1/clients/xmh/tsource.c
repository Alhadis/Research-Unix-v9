#ifndef lint
static char rcs_id[] = "$Header: tsource.c,v 1.10 87/09/11 08:18:42 toddb Exp $";
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

/* File: tsource.c -- the code for a toc source */

#include "xmh.h"
#include "tocintrnl.h"

/* Private definitions. */

#define BUFSIZE	512

Msg MsgFromPosition(toc, position, dir)
  Toc toc;
  XtTextPosition position;
  ScanDirection dir;
{
    Msg msg;
    int     h, l, m;
    if (dir == XtsdLeft) position--;
    l = 0;
    h = toc->nummsgs - 1;
    while (l < h - 1) {
	m = (l + h) / 2;
	if (toc->msgs[m]->position > position)
	    h = m;
	else
	    l = m;
    }
    msg = toc->msgs[h];
    if (msg->position > position)
	msg = toc->msgs[h = l];
    while (!msg->visible)
	msg = toc->msgs[h++];
    if (position < msg->position || position > msg->position + msg->length)
	Punt("Error in MsgFromPosition!");
    return msg;
}


static XtTextPosition CoerceToLegalPosition(toc, position)
  Toc toc;
  XtTextPosition position;
{
    return (position < 0) ? 0 :
		 ((position > toc->lastPos) ? toc->lastPos : position);
}


/* Semi-public definitions */

static TSourceReadText(src, position, text, maxRead)
  XtTextSource *src;
  XtTextPosition position;
  XtTextBlock *text;
  int maxRead;
{
    Toc toc = (Toc) src->data;
    Msg msg;
    int count;
    text->firstPos = position;
    if (position < toc->lastPos) {
	msg = MsgFromPosition(toc, position, XtsdRight);
	text->ptr = msg->buf + (position - msg->position);
	count = msg->length - (position - msg->position);
	text->length = (count < maxRead) ? count : maxRead;
	position += text->length;
    }
    else {
	text->length = 0;
	text->ptr = "";
    }
    return position;
}


/* Right now, we can only replace a piece with another piece of the same size,
   and it can't cross between lines. */

static int TSourceReplaceText(src, startPos, endPos, text, delta)
  XtTextSource *src;
  XtTextPosition startPos, endPos;
  XtTextBlock *text;
  int *delta;
{
    Toc toc = (Toc) src->data;
    Msg msg;
    int i;
    *delta = text->length - (endPos - startPos);
    if (*delta != 0)
	return EDITERROR;
    msg = MsgFromPosition(toc, startPos, XtsdRight);
    for (i = 0; i < text->length; i++)
	msg->buf[startPos - msg->position + i] = text->ptr[i];
    return EDITDONE;
}


static XtTextPosition TSourceGetLastPos(src)
  XtTextSource *src;
{
    return ((Toc) src->data)->lastPos;
}


/*ARGSUSED*/	/* -- keeps lint from complaining. */
static TSourceSetLastPos(src)
  XtTextSource *src;
{
}


#define Look(index, c)\
{									\
    if ((dir == XtsdLeft && index <= 0) ||				\
	    (dir == XtsdRight && index >= toc->lastPos))			\
	c = 0;								\
    else {								\
	if (index + doff < msg->position ||				\
		index + doff >= msg->position + msg->length)		\
	    msg = MsgFromPosition(toc, index, dir);			\
	c = msg->buf[index + doff - msg->position];			\
    }									\
}



static XtTextPosition TSourceScan(src, position, sType, dir, count, include)
  XtTextSource *src;
  XtTextPosition position;
  ScanType sType;
  ScanDirection dir;
  int count, include;
{
    Toc toc = (Toc) src->data;
    XtTextPosition index;
    Msg msg;
    char    c;
    int     ddir, doff, i, whiteSpace;
    ddir = (dir == XtsdRight) ? 1 : -1;
    doff = (dir == XtsdRight) ? 0 : -1;

    if (toc->lastPos == 0) return 0;
    index = position;
    if (index + doff < 0) return 0;
    if (dir == XtsdRight && index >= toc->lastPos) return toc->lastPos;
    msg = MsgFromPosition(toc, index, dir);
    switch (sType) {
	case XtstPositions:
	    if (!include && count > 0)
		count--;
	    index = CoerceToLegalPosition(toc, index + count * ddir);
	    break;
	case XtstWhiteSpace:
	    for (i = 0; i < count; i++) {
		whiteSpace = -1;
		while (index >= 0 && index <= toc->lastPos) {
		    Look(index, c);
		    if ((c == ' ') || (c == '\t') || (c == '\n')) {
			if (whiteSpace < 0) whiteSpace = index;
		    } else if (whiteSpace >= 0)
			break;
		    index += ddir;
		}
	    }
	    if (!include) {
		if (whiteSpace < 0 && dir == XtsdRight)
		    whiteSpace = toc->lastPos;
		index = whiteSpace;
	    }
	    index = CoerceToLegalPosition(toc, index);
	    break;
	case XtstEOL:
	    for (i = 0; i < count; i++) {
		while (index >= 0 && index <= toc->lastPos) {
		    Look(index, c);
		    if (c == '\n')
			break;
		    index += ddir;
		}
		if (i < count - 1)
		    index += ddir;
	    }
	    if (include)
		index += ddir;
	    index = CoerceToLegalPosition(toc, index);
	    break;
	case XtstFile:
	    if (dir == XtsdLeft)
		index = 0;
	    else
		index = toc->lastPos;
	    break;
    }
    return index;
}


/*ARGSUSED*/	/* -- keeps lint from complaining. */
static XtEditType TSourceGetEditType(src)
XtTextSource *src;
{
    return XttextRead;
}

/* Public definitions. */

XtTextSource *TSourceCreate(toc)
  Toc toc;
{
    XtTextSource *src;
    src = (XtTextSource *) XtMalloc(sizeof(XtTextSource));
    src->read = TSourceReadText;
    src->replace = TSourceReplaceText;
    src->getLastPos = TSourceGetLastPos;
    src->setLastPos = TSourceSetLastPos;
    src->editType = TSourceGetEditType;
    src->scan = TSourceScan;
    src->data = (int *) toc;
    return src;
}
