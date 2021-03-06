#ifndef lint
static char rcsid[] = "$Header: StringSrc.c,v 1.3 87/09/13 13:28:11 swick Exp $";
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
/* File: StringSource.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "Text.h"
#include "TextP.h"

/* Private StringSource Definitions */


typedef struct _StringSourceData {
    XtEditType editMode;
    char *str;
    XtTextPosition length, maxLength;
} StringSourceData, *StringSourcePtr;

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

static XtResource stringResources[] = {
    {XtNstring, XtCString, XrmRString, sizeof (char *),
        XtOffset(StringSourcePtr, str), XrmRString, NULL},
    {XtNlength, XtCLength, XrmRInt, sizeof (int),
        XtOffset(StringSourcePtr, length), XrmRString, "0"},
    {XtNlength, XtCLength, XrmRInt, sizeof (int),
        XtOffset(StringSourcePtr, maxLength), XrmRString, "100"},
    {XtNeditType, XtCEditType, XtREditMode, sizeof(XtEditType), 
        XtOffset(StringSourcePtr, editMode), XrmRString, "read"},
};

char Look(data, position, direction)
  StringSourcePtr data;
  XtTextPosition position;
  ScanDirection direction;
{
/* Looking left at pos 0 or right at position data->length returns newline */
    if (direction == XtsdLeft) {
	if (position == 0)
	    return(0);
	else
	    return(data->str[position-1]);
    }
    else {
	if (position == data->length)
	    return(0);
	else
	    return(data->str[position]);
    }
}

static int StringReadText (src, pos, text, maxRead)
  XtTextSource *src;
  int pos;
  XtTextBlock *text;
  int maxRead;
{
    int     charsLeft;
    StringSourcePtr data;

    data = (StringSourcePtr) src->data;
    text->firstPos = pos;
    text->ptr = data->str + pos;
    charsLeft = data->length - pos;
    text->length = (maxRead > charsLeft) ? charsLeft : maxRead;
    return pos + text->length;
}

static int StringReplaceText (src, startPos, endPos, text, delta)
  XtTextSource *src;
  XtTextPosition startPos, endPos;
  XtTextBlock *text;
  int *delta;
{
    StringSourcePtr data;
    int     i, length;

    data = (StringSourcePtr) src->data;
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
    length = endPos - startPos;
    *delta = text->length - length;
    if (*delta < 0)		/* insert shorter than delete, text getting
				   shorter */
	for (i = startPos; i < data->length + *delta; ++i)
	    data->str[i] = data->str[i - *delta];
    else
	if (*delta > 0)	{	/* insert longer than delete, text getting
				   longer */
	    for (i = data->length; i > startPos-1; --i)
		data->str[i + *delta] = data->str[i];
	}
    if (text->length != 0)	/* do insert */
	for (i = 0; i < text->length; ++i)
	    data->str[startPos + i] = text->ptr[i];
    data->length = data->length + *delta;
    data->str[data->length] = 0;
    return (EDITDONE);
}

static StringSetLastPos (src, lastPos)
  XtTextSource *src;
  XtTextPosition lastPos;
{
    ((StringSourceData *) (src->data))->length = lastPos;
}

static XtTextPosition StringScan (src, pos, sType, dir, count, include)
  XtTextSource	 *src;
  XtTextPosition pos;
  ScanType	 sType;
  ScanDirection	 dir;
  int		 count;
  Boolean	 include;
{
    StringSourcePtr data;
    XtTextPosition position;
    int     i, whiteSpace;
    char    c;
    int ddir = (dir == XtsdRight) ? 1 : -1;

    data = (StringSourcePtr) src->data;
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
		whiteSpace = -1;
		while (position >= 0 && position <= data->length) {
		    c = Look(data, position, dir);
		    if ((c == ' ') || (c == '\t') || (c == '\n')){
		        if (whiteSpace < 0) whiteSpace = position;
		    } else if (whiteSpace >= 0)
			break;
		    position += ddir;
		}
	    }
	    if (!include) {
		if(whiteSpace < 0 && dir == XtsdRight) whiteSpace = data->length;
		position = whiteSpace;
	    }
	    break;
	case XtstEOL: 
	    for (i = 0; i < count; i++) {
		while (position >= 0 && position <= data->length) {
		    if (Look(data, position, dir) == '\n')
			break;
		    if(((dir == XtsdRight) && (position == data->length)) || 
			(dir == XtsdLeft) && ((position == 0)))
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
	    if (dir == XtsdLeft)
		position = 0;
	    else
		position = data->length;
    }
    if (position < 0) position = 0;
    if (position > data->length) position = data->length;
    return(position);
}

static XtEditType StringGetEditType(src)
  XtTextSource *src;
{
    StringSourcePtr data;
    data = (StringSourcePtr) src->data;
    return(data->editMode);
} 

/***** Public routines *****/

int *XtStringSourceCreate (w, args, argCount)
    Widget w;
    ArgList args;
    int     argCount;
{
    XtTextSource *src;
    StringSourcePtr data;

    src = (XtTextSource *) XtMalloc(sizeof(XtTextSource));
    src->read = StringReadText;
    src->replace = StringReplaceText;
    src->setLastPos = StringSetLastPos;
    src->scan = StringScan;
    src->editType = StringGetEditType;
    src->data = (int *) (XtMalloc(sizeof(StringSourceData)));
    data = (StringSourcePtr) src->data;

/* XXX name and class??? */
    XtGetSubresources (XtDisplay(w), data, "subclass", "subclass",
        stringResources, XtNumber(stringResources), args, argCount);

    if (data->str == NULL) {
	data->str = (char *) XtMalloc (data->maxLength);
	data->length = 0;
    }
    src->data = (int *) (data);
    return(int *) src;
}

void XtStringSourceDestroy (src)
  XtTextSource *src;
{
    XtFree((char *) src->data);
    XtFree((char *) src);
}
