#include "copyright.h"

/* $Header: XFontNames.c,v 11.17 87/09/11 08:03:23 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/
#define NEED_REPLIES
#include "Xlibint.h"

char **XListFonts(dpy, pattern, maxNames, actualCount)
register Display *dpy;
char *pattern;  /* null-terminated */
int maxNames;
int *actualCount;	/* RETURN */
{       
    register long nbytes;
    register int i;
    register int length;
    char **flist;
    char *ch;
    xListFontsReply rep;
    register xListFontsReq *req;

    LockDisplay(dpy);
    GetReq(ListFonts, req);
    req->maxNames = maxNames;
    nbytes = req->nbytes = strlen (pattern);;
    req->length += (nbytes + 3) >> 2;
    _XSend (dpy, pattern, nbytes);
       /* use _XSend instead of Data, since following _XReply will flush buffer */

    (void) _XReply (dpy, (xReply *)&rep, 0, xFalse);
    *actualCount = rep.nFonts;
    if (*actualCount) {
	    flist = (char **)Xmalloc ((unsigned)rep.nFonts * sizeof(char *));
	    ch = (char *) Xmalloc((unsigned)(rep.length * 4) + 1);
        	/* +1 to leave room for last null-terminator */
	    _XReadPad (dpy, ch, (long)(rep.length * 4));
	    /*
	     * unpack into null terminated strings.
	     */
	    length = *ch;
	    for (i = 0; i < rep.nFonts; i++) {
		flist[i] = ch + 1;  /* skip over length */
		ch += length + 1;  /* find next length ... */
		length = *ch;
		*ch = '\0';  /* and replace with null-termination */
	    }
	}
    else flist = NULL;
    UnlockDisplay(dpy);
    SyncHandle();
    return (flist);
}

XFreeFontNames(list)
char **list;
{       
	if (list != NULL) {
		Xfree (list[0]-1);
		Xfree ((char *)list);
	}
}
