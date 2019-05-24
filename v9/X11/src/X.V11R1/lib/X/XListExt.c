#include "copyright.h"

/* $Header: XListExt.c,v 11.6 87/08/06 23:11:33 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

char **XListExtensions(dpy, nextensions)
register Display *dpy;
int *nextensions;
{
	xListExtensionsReply rep;
	char **list;
	char *ch;
	register int i;
	register int length;
	register xReq *req;
	register long rlen;

	LockDisplay(dpy);
	GetEmptyReq (ListExtensions, req);
	(void) _XReply (dpy, (xReply *) &rep, 0, xFalse);
	if(*nextensions = rep.nExtensions) {
	    list = (char **) Xmalloc (
		(unsigned)((long)*nextensions * sizeof (char *)));
	    rlen = rep.length << 2;
	    ch = (char *) Xmalloc ((unsigned) rlen + 1);
                /* +1 to leave room for last null-terminator */
	    _XReadPad (dpy, ch, rlen);
	    /*
	     * unpack into null terminated strings.
	     */
	    length = *ch;
	    for (i = 0; i < *nextensions; i++) {
		list[i] = ch+1;  /* skip over length */
		ch += length + 1; /* find next length ... */
		length = *ch;
		*ch = '\0'; /* and replace with null-termination */
	    }
	}
	else list = NULL;
	UnlockDisplay(dpy);
	SyncHandle();
	return (list);
}

XFreeExtensionList (list)
char **list;
{
	if (list != NULL) {
	    Xfree (list[0]-1);
	    Xfree ((char *)list);
	}
}
