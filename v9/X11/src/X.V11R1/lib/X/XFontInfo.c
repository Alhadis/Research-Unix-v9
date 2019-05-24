#include "copyright.h"

/* $Header: XFontInfo.c,v 11.8 87/09/11 08:08:46 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/
#define NEED_REPLIES
#include "Xlibint.h"

char **XListFontsWithInfo(dpy, pattern, maxNames, actualCount, info)
register Display *dpy;
char *pattern;  /* null-terminated */
int maxNames;
int *actualCount;
XFontStruct **info;
{       
    register long nbytes;
    register int i;
    register XFontStruct *fs;
    register int size = 0;
    XFontStruct *finfo = NULL;
    char **flist = NULL;
    xListFontsWithInfoReply reply;
    register xListFontsReq *req;

    LockDisplay(dpy);
    GetReq(ListFontsWithInfo, req);
    req->maxNames = maxNames;
    nbytes = req->nbytes = strlen (pattern);;
    req->length += (nbytes + 3) >> 2;
    _XSend (dpy, pattern, nbytes);
    /* use _XSend instead of Data, since subsequent _XReply will flush buffer */

    for (i = 0; ; i++) {
	if (!_XReply (dpy, (xReply *) &reply,
	   ((sizeof (reply) - sizeof (xGenericReply)) >> 2), xFalse))
		return (NULL);
	if (reply.nameLength == 0)
	    break;
	if ((i + reply.nReplies) >= size) {
	    size = i + reply.nReplies + 1;
	    if (finfo) {
		finfo = (XFontStruct *) Xrealloc ((char *) finfo,
						  sizeof (XFontStruct) * size);
		flist = (char **) Xrealloc ((char *) flist,
					    sizeof (char *) * size);
	    } else {
		finfo = (XFontStruct *) Xmalloc (sizeof (XFontStruct) * size);
		flist = (char **) Xmalloc (sizeof (char *) * size);
	    }
	}
	fs = &finfo[i];

	fs->ext_data 		= NULL;
	fs->per_char		= NULL;
	fs->fid 		= None;
	fs->direction 		= reply.drawDirection;
	fs->min_char_or_byte2	= reply.minCharOrByte2;
	fs->max_char_or_byte2 	= reply.maxCharOrByte2;
	fs->min_byte1 		= reply.minByte1;
	fs->max_byte1 		= reply.maxByte1;
	fs->default_char	= reply.defaultChar;
	fs->all_chars_exist 	= reply.allCharsExist;
	fs->ascent 		= reply.fontAscent;
	fs->descent 		= reply.fontDescent;
    
	/* XXX the next two statements won't work if short isn't 16 bits */

	fs->min_bounds = * (XCharStruct *) &reply.minBounds;
	fs->max_bounds = * (XCharStruct *) &reply.maxBounds;

	fs->n_properties = reply.nFontProps;
	if (fs->n_properties > 0) {
	    nbytes = reply.nFontProps * sizeof (XFontProp);
	    fs->properties = (XFontProp *) Xmalloc ((unsigned int) nbytes);
	    _XRead (dpy, (char *)fs->properties, nbytes);
	} else
	    fs->properties = NULL;
	flist[i] = (char *) Xmalloc ((unsigned int) (reply.nameLength + 1));
	flist[i][reply.nameLength] = '\0';
	_XReadPad (dpy, flist[i], reply.nameLength);
    }
    *info = finfo;
    *actualCount = i;
    UnlockDisplay(dpy);
    SyncHandle();
    return (flist);
}

XFreeFontInfo (names, info, actualCount)
char **names;
XFontStruct *info;
int actualCount;
{
	register int i;
	if (names != NULL) {
		for (i = 0; i < actualCount; i++) {
			Xfree (names[i]);
		}
		Xfree((char *) names);
	}
	if (info != NULL) {
		for (i = 0; i < actualCount; i++) {
			if (info[i].properties != NULL) 
				Xfree ((char *) info[i].properties);
			}
		Xfree((char *) info);
	}
}
