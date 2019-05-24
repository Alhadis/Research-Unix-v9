#include "copyright.h"

/* $Header: XFont.c,v 11.21 87/08/11 13:12:02 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/
#define NEED_REPLIES
#include "Xlibint.h"

XFontStruct *_XQueryFont();

XFontStruct *XLoadQueryFont(dpy, name)
   register Display *dpy;
   char *name;
{
    XFontStruct *font_result;
    register long nbytes;
    Font fid;
    xOpenFontReq *req;

    LockDisplay(dpy);
    GetReq(OpenFont, req);
    nbytes = req->nbytes  = strlen(name);
    req->fid = fid = XAllocID(dpy);
    req->length += (nbytes+3)>>2;
    Data (dpy, name, nbytes);
    dpy->request--;
    font_result = (_XQueryFont(dpy, fid));
    dpy->request++;
    if (!font_result) {
       /* if _XQueryFont returned NULL, then the OpenFont request got
          a BadName error.  This means that the following QueryFont
          request is guaranteed to get a BadFont error, since the id
          passed to QueryFont wasn't really a valid font id.  To read
          and discard this second error, we call _XReply again. */
        xReply reply;
        (void) _XReply (dpy, &reply, 0, xFalse);
        }
    UnlockDisplay(dpy);
    SyncHandle();
    return font_result;
}

XFreeFont(dpy, fs)
    register Display *dpy;
    XFontStruct *fs;
{ 
    register xResourceReq *req;
    register _XExtension *ext = dpy->ext_procs;

    LockDisplay(dpy);
    while (ext) {		/* call out to any extensions interested */
	if (ext->free_Font != NULL) (*ext->free_Font)(dpy, fs, &ext->codes);
	ext = ext->next;
	}    
    GetResReq (CloseFont, fs->fid, req);
    _XFreeExtData(fs->ext_data);
    if (fs->per_char)
       Xfree ((char *) fs->per_char);
    if (fs->properties)
       Xfree ((char *) fs->properties);
    Xfree ((char *) fs);
    ext = dpy->ext_procs;
    UnlockDisplay(dpy);
    SyncHandle();
}


XFontStruct *_XQueryFont (dpy, fid)	/* Internal-only entry point */
    register Display *dpy;
    Font fid;

{
    register XFontStruct *fs;
    register long nbytes;
    xQueryFontReply reply;
    register xResourceReq *req;
    register _XExtension *ext;

    GetResReq(QueryFont, fid, req);
    if (!_XReply (dpy, (xReply *) &reply,
       ((sizeof (reply) - sizeof (xReply)) >> 2), xFalse))
	   return (NULL);
    fs = (XFontStruct *) Xmalloc (sizeof (XFontStruct));
    fs->ext_data 		= NULL;
    fs->fid 			= fid;
    fs->direction 		= reply.drawDirection;
    fs->min_char_or_byte2	= reply.minCharOrByte2;
    fs->max_char_or_byte2 	= reply.maxCharOrByte2;
    fs->min_byte1 		= reply.minByte1;
    fs->max_byte1 		= reply.maxByte1;
    fs->default_char 		= reply.defaultChar;
    fs->all_chars_exist 	= reply.allCharsExist;
    fs->ascent 			= reply.fontAscent;
    fs->descent 		= reply.fontDescent;
    
    /* XXX the next two statements won't work if short isn't 16 bits */

    fs->min_bounds = * (XCharStruct *) &reply.minBounds;
    fs->max_bounds = * (XCharStruct *) &reply.maxBounds;

    fs->n_properties = reply.nFontProps;
    /* 
     * if no properties defined for the font, then it is bad
     * font, but shouldn't try to read nothing.
     */
    fs->properties = NULL;
    if (fs->n_properties > 0) {
	    fs->properties = (XFontProp *) Xmalloc (
	       (unsigned)(nbytes = reply.nFontProps * sizeof (XFontProp)));
	    _XRead (dpy, (char *)fs->properties, nbytes);
    }
    /*
     * If no characters in font, then it is a bad font, but
     * shouldn't try to read nothing.
     */
    /* XXX may have to unpack charinfos on some machines (CRAY) */
    fs->per_char = NULL;
    if (reply.nCharInfos > 0){
	    fs->per_char = (XCharStruct *) Xmalloc (
	       (unsigned)(nbytes = reply.nCharInfos * sizeof (XCharStruct)));
	    _XRead (dpy, (char *)fs->per_char, nbytes);
    }

    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
	if (ext->create_Font != NULL) 
		(*ext->create_Font)(dpy, fs, &ext->codes);
	ext = ext->next;
	}    
    return (fs);
}


XFontStruct *XQueryFont (dpy, fid)
    register Display *dpy;
    Font fid;
{
    XFontStruct *font_result;

    LockDisplay(dpy);
    font_result = _XQueryFont(dpy,fid);
    UnlockDisplay(dpy);
    SyncHandle();
    return font_result;
}

   
   
