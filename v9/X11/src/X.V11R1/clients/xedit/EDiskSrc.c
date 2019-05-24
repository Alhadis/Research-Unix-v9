#ifndef lint
static char rcs_id[] = "$Header: EDiskSrc.c,v 1.5 87/09/11 08:21:59 toddb Exp $";
#endif

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

/* File: TEDiskSource.c - last edit by */
/* weissman:	25 Aug 86 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "xedit.h"
/* Private definitions. */

#define BUFSIZE	512

extern char *malloc();
extern char *realloc();
extern char *strcpy();

typedef struct {
    char *buf;
    int length;
    int maxlength;
    long start;
} PieceRec, *PiecePtr;

typedef struct {
    int file;
    char *name;
    XtTextPosition length;
    XtTextPosition origlength;
    int numpieces;
    PiecePtr piece;
    short changed, everchanged;
    short startsvalid;
    short eversaved;
    XtEditType editMode;
} EDiskRec, *EDiskPtr;


static CalculateStarts(data)
  EDiskPtr data;
{
    int     i;
    PiecePtr piece;
    long    start = 0;
    for (i = 0, piece = data->piece; i < data->numpieces; i++, piece++) {
	piece->start = start;
	start += piece->length;
    }
    data->startsvalid = TRUE;
}

static ReadPiece(data, i)
  EDiskPtr data;
  int i;
{
    PiecePtr piece = data->piece + i;
    piece->maxlength = piece->length;
    piece->buf = malloc(piece->maxlength);
    lseek(data->file, i * BUFSIZE, 0);
    read(data->file, piece->buf, piece->length);
}



static PiecePtr PieceFromPosition(data, position)
  EDiskPtr data;
  XtTextPosition position;
{
    int     i;
    PiecePtr piece;
    if (!data->everchanged) {
	i = position / BUFSIZE;
	if (i >= data->numpieces) i = data->numpieces - 1;
	piece = data->piece + i;
    } else {
	if (!data->startsvalid)
	    CalculateStarts(data);
	for (i = 0, piece = data->piece; i < data->numpieces - 1; i++, piece++)
	    if (position < piece->start + piece->length)
		break;
    }
    if (!piece->buf) ReadPiece(data, i);
    return piece;
}

static XtTextPosition CoerceToLegalPosition(data, position)
  EDiskPtr data;
  XtTextPosition position;
{
    return (position < 0) ? 0 :
		 ((position > data->length) ? data->length : position);
}


/* Semi-public definitions */

static EDiskReadText(src, position, text, maxRead)
  XtTextSource *src;
  XtTextPosition position;
  XtTextBlock *text;
  int maxRead;
{
    EDiskPtr data = (EDiskPtr) src->data;
    PiecePtr piece;
    int     count;
    text->firstPos = position;
    if (position < data->length) {
	piece = PieceFromPosition(data, position);
	text->ptr = piece->buf + (position - piece->start);
	count = piece->length - (position - piece->start);
	text->length = (count < maxRead) ? count : maxRead;
    }
    else {
	text->length = 0;
	text->ptr = "";
    }
    return position + text->length;
}


static EDiskReplaceText(src, startPos, endPos, text, Delta)
  XtTextSource *src;
  XtTextPosition startPos, endPos;
  XtTextBlock *text;
  int *Delta;
{
    EDiskPtr data = (EDiskPtr) src->data;
    PiecePtr piece, piece2;
    int     oldlength = endPos - startPos;
    int     delta = text->length - oldlength;
    int     i;
   switch (data->editMode) {
        case XttextAppend:
            if (startPos != endPos || endPos!= data->length)
                return (POSITIONERROR);
            break;
        case XttextRead:
            return (EDITERROR);
        case XttextEdit:
            break;
        default:
            return (EDITERROR);
    }

    if (delta == 0 && oldlength == 0) 
	return 0;
    data->length += delta;
    piece = PieceFromPosition(data, startPos);
    if (oldlength > 0) {
	piece2 = PieceFromPosition(data, endPos - 1);
	if (piece != piece2) {
	    oldlength = endPos - piece2->start;
	    piece2->length -= oldlength;
	    for (i = 0; i < piece2->length; i++)
		piece2->buf[i] = piece2->buf[i + oldlength];
	    for (piece2--; piece2 > piece; piece2--)
		piece2->length = 0;
	    oldlength = piece->length - (startPos - piece->start);
	    delta = text->length - oldlength;
	}
    }
    data->changed = data->everchanged = TRUE;
    data->startsvalid = FALSE;
    piece->length += delta;
    if (piece->length > piece->maxlength) {
	do
	    piece->maxlength *= 2;
	while (piece->length > piece->maxlength);
	piece->buf = realloc(piece->buf, piece->maxlength);
    }
    if (delta < 0)		/* insert shorter than delete, text getting
				   shorter */
	for (i = startPos - piece->start; i < piece->length; i++)
	    piece->buf[i] = piece->buf[i - delta];
    else
	if (delta > 0)		/* insert longer than delete, text getting
				   longer */
	    for (i = piece->length - delta;
		    i >= startPos - piece->start;
		    i--)
		piece->buf[i + delta] = piece->buf[i];
    if (text->length)	/* do insert */
	for (i = 0; i < text->length; ++i)
	    if ((piece->buf[startPos - piece->start + i] = text->ptr[i]) == CR)
		 piece->buf[startPos - piece->start + i] = LF;
	*Delta = text->length - (endPos - startPos);
    return EDITDONE;
}


static XtTextPosition EDiskGetLastPos(src)
  XtTextSource *src;
{
    return ((EDiskPtr) src->data)->length;
}


static EDiskSetLastPos(src)
  XtTextSource *src;
{
}

#define Look(index, c)\
{									\
    if ((dir == XtsdLeft && index <= 0) ||				\
	    (dir == XtsdRight && index >= data->length))			\
	c = 0;								\
    else {								\
	if (index + doff < piece->start ||				\
		index + doff >= piece->start + piece->length)		\
	    piece = PieceFromPosition(data, index + doff);		\
	c = piece->buf[index + doff - piece->start];			\
    }									\
}



static XtTextPosition EDiskScan(src, position, sType, dir, count, include)
  XtTextSource *src;
  XtTextPosition position;
  ScanType sType;
  ScanDirection dir;
  int count, include;
{
    EDiskPtr data = (EDiskPtr) src->data;
    XtTextPosition index;
    PiecePtr piece;
    char    c;
    int     ddir, doff, i, whiteSpace;
    ddir = (dir == XtsdRight) ? 1 : -1;
    doff = (dir == XtsdRight) ? 0 : -1;
    
    index = position;
    piece = data->piece;
    switch (sType) {
	case XtstPositions: 
	    if (!include && count > 0)
		count--;
	    index = CoerceToLegalPosition(data, index + count * ddir);
	    break;
	case XtstWhiteSpace: 	/* %%% This seems to ignore the fact that
				   whitespace can come in big chunks */
	    for (i = 0; i < count; i++) {
		whiteSpace = -1;
		while (index >= 0 && index <= data->length) {
		    Look(index, c);
		    if ((c == ' ') || (c == '\t') || (c == '\n')) {
			if (whiteSpace < 0) whiteSpace = index;
		    } else if (whiteSpace >= 0)
			break;
		    index += ddir;
		}
	    }
	    if (!include) {
		if (whiteSpace < 0 && dir == XtsdRight) whiteSpace = data->length;
		index = whiteSpace;
	    }
	    index = CoerceToLegalPosition(data, index);
	    break;
	case XtstEOL: 
	    for (i = 0; i < count; i++) {
		while (index >= 0 && index <= data->length) {
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
	    index = CoerceToLegalPosition(data, index);
	    break;
	case XtstFile: 
	    if (dir == XtsdLeft)
		index = 0;
	    else
		index = data->length;
	    break;
    }
    return index;
}

static XtEditType TEDiskGetEditType(src)
  XtTextSource *src;
{
    EDiskPtr data = (EDiskPtr) src->data;
    return(data->editMode);
}


/* Public definitions. */

XtTextSource *TCreateEDiskSource(name, mode)
  char *name;
  XtEditType mode;
{
    XtTextSource * src;
    EDiskPtr data;
    struct stat stat;
    int     i;
    src = (XtTextSource *) malloc(sizeof(XtTextSource));
    src->read = EDiskReadText;
    src->replace = EDiskReplaceText;
    src->getLastPos = EDiskGetLastPos;
    src->setLastPos = EDiskSetLastPos;
    src->editType = TEDiskGetEditType;
    src->scan = EDiskScan;
    data = (EDiskPtr) malloc(sizeof(EDiskRec));
    src->data = (int *) data;
    data->file = open(name, O_RDONLY | O_CREAT, 0666);
    data->name = strcpy(malloc(strlen(name) + 1), name);
    fstat(data->file, &stat);
    data->length = data->origlength = stat.st_size;
    data->numpieces = (data->length + BUFSIZE - 1) / BUFSIZE;
    if (data->numpieces < 1)
	data->numpieces = 1;
    data->piece = (PiecePtr) malloc(data->numpieces * sizeof(PieceRec));
    for (i = 0; i < data->numpieces; i++) {
	data->piece[i].buf = 0;
	data->piece[i].length = BUFSIZE;
	data->piece[i].start = i * BUFSIZE;
    }
    data->piece[data->numpieces - 1].length -=
	data->numpieces * BUFSIZE - data->length;
    data->startsvalid = TRUE;
    data->changed = data->everchanged = data->eversaved = FALSE;
    if (data->length == 0) {
	data->piece[0].buf = malloc(1);
	data->piece[0].maxlength = 1;
    }
    data->editMode = mode;
    return src;
}


TEDiskChanged(src)
  XtTextSource *src;
{
    return ((EDiskPtr) src->data)->changed;
}

TEDiskSaveFile(src)
  XtTextSource *src;
{
    EDiskPtr data = (EDiskPtr) src->data;
    PiecePtr piece;
    int     i, needlseek;
    if (!data->changed)
	return;
    data->changed = FALSE;
    if (!data->eversaved) {
	data->eversaved = TRUE;
	close(data->file);
	data->file = open(data->name, O_RDWR, 0666);
    }
    if (!data->startsvalid)
	CalculateStarts(data);
    for (i = 0, piece = data->piece; i < data->numpieces; i++, piece++) {
	if (!(piece->buf) && i * BUFSIZE != piece->start)
	    ReadPiece(data, i);
    }
    needlseek = TRUE;
    for (i = 0, piece = data->piece; i < data->numpieces; i++, piece++) {
	if (piece->buf) {
	    if (needlseek) {
		lseek(data->file, piece->start, 0);
		needlseek = FALSE;
	    }
	    write(data->file, piece->buf, piece->length);
	}
	else
	    needlseek = TRUE;
    }
    if (data->length < data->origlength)
	ftruncate(data->file, data->length);
    data->origlength = data->length;
}

TEDiskSaveAsFile(src, filename)
  XtTextSource *src;
  char *filename;
{
    EDiskPtr data = (EDiskPtr) src->data;
    int     i;
    PiecePtr piece;
    if (strcmp(filename, data->name) != 0) {
	for (i = 0, piece = data->piece; i < data->numpieces; i++, piece++)
	    if (!(piece->buf))
		ReadPiece(data, i);
	data->eversaved = FALSE;
	data->origlength = 0;
	data->file = creat(filename, 0666);
	data->eversaved = TRUE;
	data->origlength = 0;
	free(data->name);
	data->name = strcpy(malloc(strlen(filename) + 1), filename);
	data->changed = TRUE;
    }
    TEDiskSaveFile(src);
}


TDestroyEDiskSource(src)
  XtTextSource *src;
{
    EDiskPtr data = (EDiskPtr) src->data;
    int i;
    char *ptr;
    close(data->file);
    for (i=0 ; i<data->numpieces ; i++)
	if (ptr = data->piece[i].buf) free(ptr);
    free(data->piece);
    free(data->name);
    free(data);
    free(src);
}
