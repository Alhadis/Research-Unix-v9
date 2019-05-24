/* $Header: DiskSrc.c,v 1.1 87/09/11 08:00:03 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)DiskSource.c	1.9	2/25/87";
#endif lint

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/* File: DiskSource.c */
/* Documentation for source specfic routine semantics may be found in the
 * TextDisp.h file.
 */

#include "Xlib.h"
#include <stdio.h>
#include "Intrinsic.h"
#include "Text.h"
#include "TextDisp.h"   /** included in all text subwindow files **/

void bcopy();

/** private DiskSource definitions **/

typedef struct _DiskSourceData {
    FILE *file;		
    XtTextPosition position, 	/* file position of first char in buffer */
 		  length; 	/* length of file */
    char *buffer;		/* piece of file in memory */
    int charsInBuffer;		/* number of bytes used in memory */
    XtEditType editMode;		/* append, read */
} DiskSourceData, *DiskSourcePtr;

#define bufSize 1000

#define Increment(data, position, direction)\
{\
    if (direction == XtsdLeft) {\
	if (position > 0) \
	    position -= 1;\
    }\
    else {\
	if (position < data->length)\
	    position += 1;\
    }\
}

static char Look(data, position, direction)
  DiskSourcePtr data;
  XtTextPosition position;
  ScanDirection direction;
{

    if (direction == XtsdLeft) {
	if (position == 0)
	    return('\n');
	else {
	    FillBuffer(data, position - 1);
	    return(data->buffer[position - data->position - 1]);
	}
    }
    else {
	if (position == data->length)
	    return('\n');
	else {
	    FillBuffer(data, position);
	    return(data->buffer[position - data->position]);
	}
    }
}



int DiskReadText (src, pos, text, maxRead)
  XtTextSource *src;
  XtTextPosition pos;	/** starting position */
  XtTextBlock *text;	/** RETURNED: text read in */
  int maxRead;		/** max number of bytes to read **/
{
    XtTextPosition count;
    DiskSourcePtr data;

    data = (DiskSourcePtr) src->data;
    FillBuffer(data, pos);
    text->firstPos = pos;
    text->ptr = data->buffer + (pos - data->position);
    count = data->charsInBuffer - (pos - data->position);
    text->length = (maxRead > count) ? count : maxRead;
    return pos + text->length;
}

/*
 * this routine reads text starting at "pos" into memory.
 * Contains heuristic for keeping the read position centered in the buffer.
 */
static int FillBuffer (data, pos)
  DiskSourcePtr data;
  XtTextPosition pos;
{
    long readPos;
    if ((pos < data->position ||
	    pos >= data->position + data->charsInBuffer - 100) &&
	    data->charsInBuffer != data->length) {
	if (pos < (bufSize / 2))
	    readPos = 0;
	else
	    if (pos >= data->length - bufSize)
		readPos = data->length - bufSize;
	    else
		if (pos >= data->position + data->charsInBuffer - 100)
		    readPos = pos - (bufSize / 2);
		else
		    readPos = pos;
	(void) fseek(data->file, readPos, 0);
	data->charsInBuffer = fread(data->buffer, sizeof(char), bufSize,
				data->file);
	data->position = readPos;
    }
}

/*
 * This is a dummy routine for read only disk sources.
 */
/*ARGSUSED*/  /* keep lint happy */
static int DummyReplaceText (src, startPos, endPos, text, delta)
  XtTextSource *src;
  XtTextPosition startPos, endPos;
  XtTextBlock *text;
  int *delta;
{
    return(EDITERROR);
}


/*
 * This routine will only append to the end of a source.  If incorrect
 * starting and ending positions are given, an error will be returned.
 */
static int DiskAppendText (src, startPos, endPos, text, delta)
  XtTextSource *src;
  XtTextPosition startPos, endPos;
  XtTextBlock *text;
  int *delta;
{
    long topPosition = 0;
    char *tmpPtr;
    DiskSourcePtr data;
    data = (DiskSourcePtr) src->data;
    if (startPos != endPos || endPos != data->length)
        return (POSITIONERROR);
    /* write the new text to the end of the file */
    if (text->length > 0) {
	(void) fseek(data->file, data->length, 0);
	(void) fwrite(text->ptr, sizeof(char), text->length, data->file);
    } else
	/* if the delete key was hit, blank out last char in the file */
	if (text->length < 0) {
		(void) fseek(data->file, data->length-1, 0);
		(void) fwrite(" ", sizeof(char), 1, data->file);
	}
    /* need this in case the application trys to seek to end of file. */
     (void) fseek(data->file, topPosition, 2);	
     
    /* put the new text into the buffer in memory */
    data->length += text->length;
    if (data->charsInBuffer + text->length <= bufSize) {
/**** NOTE: need to check if text won't fit in the buffer ***/
	if (text->length > 0) {
		tmpPtr = data->buffer + data->charsInBuffer;
		bcopy(text->ptr, tmpPtr, text->length);
	}
	data->charsInBuffer += text->length;
    } else
	FillBuffer(data, data->length - text->length);

    *delta = text->length;
    return (EDITDONE);
}


static int DiskSetLastPos (src, lastPos)
  XtTextSource *src;
  XtTextPosition lastPos;
{
    ((DiskSourceData *)(src->data))->length = lastPos;
}

/*
 * This routine will start at
 * the "pos" position of the source and scan in the appropriate
 * direction until it finds something of the right sType.  It returns 
 * the new position.  If upon reading it hits the end of the buffer
 * in memory, it will refill the buffer.
 */
static XtTextPosition DiskScan (src, pos, sType, dir, count, include)
  XtTextSource 	 *src;
  XtTextPosition pos;
  ScanType 	 sType;
  ScanDirection  dir;
  int     	 count;
  Boolean	 include;
{
    DiskSourcePtr data;
    XtTextPosition position;
    int     i, whiteSpace;
    char    c;

    data = (DiskSourcePtr) src->data;
    position = pos;
    switch (sType) {
	case XtstPositions: 
	    if (!include && count > 0)
		count -= 1;
	    for (i = 0; i < count; i++) {
		Increment(data, position, dir);
	    }
	    break;
	case XtstWhiteSpace: 
	    for (i = 0; i < count; i++) {
		whiteSpace = 0;
		while (position >= 0 && position <= data->length) {
		    FillBuffer(data, position);
		    c = Look(data, position, dir);
		    whiteSpace = (c == ' ') || (c == '\t') || (c == '\n');
		    if (whiteSpace)
			break;
		    Increment(data, position, dir);
		}
		if (i + 1 != count)
		    Increment(data, position, dir);
	    }
	    if (include)
		Increment(data, position, dir);
	    break;
	case XtstEOL: 
	    for (i = 0; i < count; i++) {
		while (position >= 0 && position <= data->length) {
		    if (Look(data, position, dir) == '\n')
			break;
		    Increment(data, position, dir);
		}
		if (i + 1 != count)
		    Increment(data, position, dir);
	    }
	    if (include) {
	    /* later!!!check for last char in file # eol */
		Increment(data, position, dir);
	    }
	    break;
	case XtstFile: 
	case XtselectAll: 
	    if (dir == XtsdLeft)
		position = 0;
	    else
		position = data->length;
    }
    return(position);
}


static XtEditType DiskGetEditType(src)
  XtTextSource *src;
{
    DiskSourcePtr data;
    data = (DiskSourcePtr) src->data;
    return(data->editMode);
}

/******* Public routines **********/

int *XtDiskSourceCreate(name, mode)
    char       *name;
    XtEditType mode;
{
    XtTextSource *src;
    DiskSourcePtr data;
    long topPosition = 0;

    src = (XtTextSource *) XtMalloc(sizeof(XtTextSource));
    src->read = DiskReadText;
    src->setLastPos = DiskSetLastPos;
    src->scan = DiskScan;
    src->editType = DiskGetEditType;
    src->data = (int *) (XtMalloc(sizeof(DiskSourceData)));
    data = (DiskSourcePtr) src->data;
    switch (mode) {
        case XttextRead:
           if ((data->file = fopen(name, "r")) == 0)
                XtErrorFunction(XtFOPEN);
            src->replace = DummyReplaceText;
            break;
        case XttextAppend:
            if ((data->file = fopen(name, "r+")) == 0)
                XtErrorFunction(XtFOPEN);
            src->replace = DiskAppendText;
            break;
        default:
            if ((data->file = fopen(name, "r")) == 0)
                XtErrorFunction(XtFOPEN);
            src->replace = DummyReplaceText;
    }
    (void) fseek(data->file, topPosition, 2);  
    data->editMode = mode;
    data->length = ftell (data->file);  
    data->buffer = (char *) XtMalloc(bufSize);
    data->position = 0;
    data->charsInBuffer = 0;
    src->data = (int *) (data);
    return(int *) src;
}

void XtDiskSourceDestroy (src)
  XtTextSource *src;
{
    DiskSourcePtr data;
    data = (DiskSourcePtr) src->data;
    XtFree((char *) data->buffer);
    XtFree((char *) src->data);
    XtFree((char *) src);
}
