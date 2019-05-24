#ifndef lint
static char rcs_id[] = "$Header: EDiskSrc.c,v 1.13 87/09/11 08:19:00 toddb Exp $";
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

/* File: EDiskSource.c */

/* This is a rather simplistic text source.  It allows editing of a disk file.
   It divides the file into blocks of BUFSIZE characters.  No block is
   actually read in from disk until referenced; each block will be read at
   most once.  Thus, the entire file can eventually be read into memory and
   stored there.  When the save function is called, everything gets written
   back. */
   


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "xmh.h"


/* Private definitions. */

#define BUFSIZE	512		/* Number of chars to store in each piece. */

extern char *strcpy();
extern long lseek();

/* The structure corresponding to each piece of the file in memory. */

typedef struct {
    char *buf;			/* Pointer to this piece's data. */
    int length;			/* Number of chars used in buf. */
    int maxlength;		/* Number of chars allocated in buf. */
    long start;			/* Position in file corresponding to piece. */
} PieceRec, *PiecePtr;



/* The master structure for the source. */

typedef struct {
    int file;			/* File descriptor. */
    char *name;			/* Name of file. */
    int length;			/* Number of characters in file. */
    int origlength;		/* Number of characters originally in file. */
    int numpieces;		/* How many pieces the file is divided in. */
    PiecePtr piece;		/* Pointer to array of pieces. */
    short changed;		/* If changes have not been written to disk. */
    short everchanged;		/* If changes have ever been made. */
    short startsvalid;		/* True iff the start fields in pieces
				   are correct. */
    short eversaved;		/* If we've ever saved changes to the file. */
    XtEditType editMode;	/* Whether we are editing or read-only. */
    void (*func)();		/* Function to call when a change is made. */
    caddr_t param;		/* Parameter to pass to above function. */
    int checkpointed;		/* Filename to store checkpoints. */
    int checkpointchange;	/* TRUE if we've changed since checkpointed. */
} EDiskRec, *EDiskPtr;



/* Refigure the starting location of each piece, which is the sum of the
   lengths of all the preceding pieces. */

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



/* Read in the text for the ith piece. */

static ReadPiece(data, i)
  EDiskPtr data;
  int i;
{
    PiecePtr piece = data->piece + i;
    piece->maxlength = piece->length;
    piece->buf = XtMalloc((unsigned) piece->maxlength);
    (void)lseek(data->file, (long) i * BUFSIZE, 0);
    (void)read(data->file, piece->buf, piece->length);
}



/* Figure out which piece corresponds to the given position.  If this source
   has never had any changes, then this is a simple matter of division;
   otherwise, we have to search for the specified piece.  (Currently, this is
   a simple linear search; a binary one would probably be better.) */

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


/* Return the given position, coerced to be within the range of legal positions
   for the given source. */

static XtTextPosition CoerceToLegalPosition(data, position)
  EDiskPtr data;
  XtTextPosition position;
{
    return (position < 0) ? 0 :
		 ((position > data->length) ? data->length : position);
}



/* Semi-public definitions */

/* Reads in the text for the given range.  Will not read past a piece boundary.
   Returns the position after the last character that was read. */

static EDiskReadText(src, position, text, maxRead)
  XtTextSource *src;
  XtTextPosition position;	/* Starting position to read. */
  XtTextBlock *text;		/* RETURN - the text read. */
  int maxRead;			/* Number of positions to read. */
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




/* Replace the text between startPos and endPos with the given text.  Returns
   EDITDONE on success, or EDITERROR if this source is read-only.  */


static int EDiskReplaceText(src, startPos, endPos, text, delta)
  XtTextSource *src;
  XtTextPosition startPos, endPos;
  XtTextBlock *text;
  int *delta;			/* RETURN - change in length of source. */
{
    EDiskPtr data = (EDiskPtr) src->data;
    PiecePtr piece, piece2;
    int     oldlength = endPos - startPos;
    int     i;
    int     del = text->length - oldlength;
    if (del == 0 && oldlength == 0) {
	*delta = 0;
	return EDITDONE;
    }
    if (data->editMode != XttextEdit)
	return EDITERROR;
    data->length += del;
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
	    del = text->length - oldlength;
	}
    }
    data->changed = data->everchanged = data->checkpointchange = TRUE;
    data->startsvalid = FALSE;
    piece->length += del;
    if (piece->length > piece->maxlength) {
	do
	    piece->maxlength *= 2;
	while (piece->length > piece->maxlength);
	piece->buf = XtRealloc(piece->buf, (unsigned)piece->maxlength);
    }
    if (del < 0)		/* insert shorter than delete, text getting
				   shorter */
	for (i = startPos - piece->start; i < piece->length; i++)
	    piece->buf[i] = piece->buf[i - del];
    else
	if (del > 0)		/* insert longer than delete, text getting
				   longer */
	    for (i = piece->length - del - 1;
		    i >= startPos - piece->start;
		    i--)
		piece->buf[i + del] = piece->buf[i];
    if (text->length)	/* do insert */
	for (i = 0; i < text->length; ++i)
	    if ((piece->buf[startPos - piece->start + i] = text->ptr[i]) == CR)
		 piece->buf[startPos - piece->start + i] = LF;
    *delta = text->length - (endPos - startPos);
    if (data->func) (*data->func)(data->param);
    return EDITDONE;
}


/* Returns the last legal position in the source. */

static XtTextPosition EDiskGetLastPos(src)
  XtTextSource *src;
{
    return ((EDiskPtr) src->data)->length;
}


/* Sets the last legal position in the source.  This is actually an obsolete
   source request, and is therefore a noop. */

/*ARGSUSED*/	/* -- keeps lint from complaining. */
static EDiskSetLastPos(src)
  XtTextSource *src;
{
}


/* Utility macro used in EDiskScan.  Used to determine what the next character
   "after" index is, where "after" is either left or right, depending on
   what kind of search we're doing. */

#define Look(index, c)\
{									\
    if ((dir == XtsdLeft && index <= 0) ||				\
	    (dir == XtsdRight && index >= data->length))		\
	c = 0;								\
    else {								\
	if (index + doff < piece->start ||				\
		index + doff >= piece->start + piece->length)		\
	    piece = PieceFromPosition(data, index + doff);		\
	c = piece->buf[index + doff - piece->start];			\
    }									\
}



/* Scan the source.  This gets complicated, but should be explained wherever
   it is that we documented sources. */

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
    if (!piece->buf) ReadPiece(data, 0);
    switch (sType) {
	case XtstPositions:
	    if (!include && count > 0)
		count--;
	    index = CoerceToLegalPosition(data, index + count * ddir);
	    break;
	case XtstWhiteSpace:
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
		if (whiteSpace < 0 && dir == XtsdRight)
		    whiteSpace = data->length;
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


/* Return what the editting mode of this source is. */

static XtEditType EDiskGetEditType(src)
XtTextSource *src;
{
    EDiskPtr data = (EDiskPtr) src->data;
    return data->editMode;
}



/* Public definitions. */

/* Create a new text source from the given file, and the given edit mode. */

XtTextSource *XtCreateEDiskSource(name, mode)
  char *name;
  XtEditType mode;
{
    XtTextSource * src;
    EDiskPtr data;
    struct stat stat;
    int     i;
    src = (XtTextSource *) XtMalloc(sizeof(XtTextSource));
    src->read = EDiskReadText;
    src->replace = EDiskReplaceText;
    src->getLastPos = EDiskGetLastPos;
    src->setLastPos = EDiskSetLastPos;
    src->editType = EDiskGetEditType;
    src->scan = EDiskScan;
    data = (EDiskPtr) XtMalloc(sizeof(EDiskRec));
    src->data = (int *) data;
    data->editMode = mode;
    data->file = myopen(name, O_RDONLY | O_CREAT, 0666);
    data->name = strcpy(XtMalloc((unsigned)strlen(name) + 1), name);
    (void)fstat(data->file, &stat);
    data->length = data->origlength = stat.st_size;
    data->numpieces = (data->length + BUFSIZE - 1) / BUFSIZE;
    if (data->numpieces < 1)
	data->numpieces = 1;
    data->piece = (PiecePtr)
	XtMalloc((unsigned) data->numpieces * sizeof(PieceRec));
    for (i = 0; i < data->numpieces; i++) {
	data->piece[i].buf = 0;
	data->piece[i].length = BUFSIZE;
	data->piece[i].start = i * BUFSIZE;
    }
    data->piece[data->numpieces - 1].length -=
	data->numpieces * BUFSIZE - data->length;
    data->startsvalid = TRUE;
    data->changed = data->everchanged = data->eversaved = FALSE;
    data->checkpointed = data->checkpointchange = FALSE;
    data->func = NULL;
    if (data->length == 0) {
	data->piece[0].buf = XtMalloc(1);
	data->piece[0].maxlength = 1;
    }
    return src;
}



/* Change the editing mode of a source. */

XtEDiskChangeEditMode(src, editMode)
XtTextSource *src;
XtEditType editMode;
{
    EDiskPtr data = (EDiskPtr) src->data;
    data->editMode = editMode;
}



/* Call the given function with the given parameter whenever the source is
   changed. */

XtEDiskSetCallbackWhenChanged(src, func, param)
XtTextSource *src;
void (*func)();
caddr_t param;
{
    EDiskPtr data = (EDiskPtr) src->data;
    data->func = func;
    data->param = param;
}


/* Return whether any unsaved changes have been made. */

XtEDiskChanged(src)
  XtTextSource *src;
{
    return ((EDiskPtr) src->data)->changed;
}


/* Save any unsaved changes. */

XtEDiskSaveFile(src)
  XtTextSource *src;
{
    EDiskPtr data = (EDiskPtr) src->data;
    PiecePtr piece;
    int     i, needlseek;
    char str[1024];
    if (!data->changed)
	return;
    data->changed = data->checkpointchange = FALSE;
    if (!data->eversaved) {
	data->eversaved = TRUE;	/* Open file in a mode where we can write. */
	(void)myclose(data->file);
	data->file = myopen(data->name, O_RDWR, 0666);
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
		(void) lseek(data->file, (long) piece->start, 0);
		needlseek = FALSE;
	    }
	    (void) write(data->file, piece->buf, piece->length);
	}
	else
	    needlseek = TRUE;
    }
    if (data->length < data->origlength)
	(void)ftruncate(data->file, data->length);
    data->origlength = data->length;
    if (data->checkpointed) {
	(void) sprintf(str, "%s.CKP", data->name);
	(void) unlink(str);
	data->checkpointed = FALSE;
    }
}


/* Save into a new file. */

XtEDiskSaveAsFile(src, filename)
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
	XtFree(data->name);
	data->name = strcpy(XtMalloc((unsigned) strlen(filename) + 1),
			    filename);
	data->changed = TRUE;
    }
    XtEDiskSaveFile(src);
}



/* Checkpoint this file. */

XtEDiskMakeCheckpoint(src)
  XtTextSource *src;
{
    EDiskPtr data = (EDiskPtr) src->data;
    char name[1024];
    int fid, i;
    PiecePtr piece;
    if (!data->checkpointchange) return; /* No changes to save. */
    (void) sprintf(name, "%s.CKP", data->name);
    fid = creat(name, 0600);
    if (fid < 0) return;
    for (i = 0, piece = data->piece; i < data->numpieces; i++, piece++) {
	if (!(piece->buf)) ReadPiece(data, i);
	(void) write(fid, piece->buf, piece->length);
    }
    (void)close(fid);
    data->checkpointed = TRUE;
    data->checkpointchange = FALSE;
}


/* We're done with this source, XtFree its storage. */

XtDestroyEDiskSource(src)
  XtTextSource *src;
{
    EDiskPtr data = (EDiskPtr) src->data;
    int i;
    char *ptr;
    (void)myclose(data->file);
    for (i=0 ; i<data->numpieces ; i++)
	if (ptr = data->piece[i].buf) XtFree(ptr);
    XtFree((char *)data->piece);
    XtFree((char *)data->name);
    XtFree((char *)data);
    XtFree((char *)src);
}
