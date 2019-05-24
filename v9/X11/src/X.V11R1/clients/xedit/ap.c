#ifndef lint
static char rcs_id[] = "$Header: ap.c,v 1.6 87/09/11 08:22:04 toddb Exp $";
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "xedit.h"

#define chunk 2048

typedef struct {
    char *buf;
    int size;
    XtTextPosition pos;
    XtTextSource *strSrc;
} ApAsSourceData;

/* Private Routines */

static XtTextPosition ApAsGetLastPos(src)
  XtTextSource *src;
{
  ApAsSourceData *data;
    data = (ApAsSourceData *)src->data;
    return ((*data->strSrc->getLastPos)(data->strSrc));
}

static ApAsSetLastPos(src, lastPos)
  XtTextSource *src;
  XtTextPosition lastPos;
{
}

static int ApAsRead(src, pos, text, maxRead)
  XtTextSource *src;
  int pos;
  XtTextBlock *text;
  int maxRead;
{
  ApAsSourceData *data;
    data = (ApAsSourceData *)src->data;
    return ((*data->strSrc->read)(data->strSrc, pos, text, maxRead));
}


static int ApAsReplace(src, startPos, endPos, text, delta)
  XtTextSource *src;
  XtTextPosition startPos, endPos;
  XtTextBlock *text;
  int *delta;
{
  ApAsSourceData *data;
  int i;
    data = (ApAsSourceData *)src->data;
    if((startPos!=endPos) || (startPos!=data->pos))
        return 0;
    if((data->pos + text->length) >= data->size){
        while((data->pos + text->length) >= data->size){
            data->size += chunk;
            data->buf = realloc(data->buf, data->size);  /* optimize this!!! */
        } 
        XtStringSourceDestroy(data->strSrc);
        data->strSrc = (XtTextSource *)XtStringSourceCreate(data->buf, 
						data->size, XttextEdit);
    }
    i = (*data->strSrc->replace)(data->strSrc, startPos, endPos, text, delta);
    data->pos += *delta;
    return (i);
}

static XtTextPosition ApAsScan (src, pos, sType, dir, count, include)
  XtTextSource *src;
  XtTextPosition pos;
  ScanType sType;
  ScanDirection dir;
  int count, include;
{
  ApAsSourceData *data;
    data = (ApAsSourceData *)src->data;
    return 
     ((*data->strSrc->scan)(data->strSrc, pos, sType, dir, count, include));
}
static XtEditType ApAsGetEditType(src)
  XtTextSource *src;
{
/*
    StringSourcePtr data;
    data = (StringSourcePtr) src->data;
    return(data->editMode);
*/
    return(XttextAppend);
}


/* Public routines */

XtTextSource *TCreateApAsSource ()
{
  XtTextSource *src;
  ApAsSourceData *data;
    src = (XtTextSource *) malloc(sizeof(XtTextSource));
    src->read = ApAsRead;
    src->replace = ApAsReplace; 
    src->getLastPos = ApAsGetLastPos;
    src->setLastPos = ApAsSetLastPos;
    src->scan = ApAsScan;
    src->editType = ApAsGetEditType;
    data = (ApAsSourceData *)(malloc(sizeof(ApAsSourceData)));
    data->buf = calloc(chunk,1);
    data->pos = 0;
    data->size = chunk;
    data->strSrc = (XtTextSource *) XtStringSourceCreate (data->buf, data->size,XttextEdit);
    src->data = (int *)data;
    return src;
}

TDestroyApAsSource(src)
  XtTextSource *src;
{
  ApAsSourceData *data;
    data = (ApAsSourceData *)src->data;
    free(data->buf);
    free(data);
    free(src);
}
