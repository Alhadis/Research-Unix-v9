#include "copyright.h"

/* $Header: XModMap.c,v 11.3 87/09/13 00:32:22 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

XModifierKeymap *
XGetModifierMapping(dpy)
     register Display *dpy;
{       
    xGetModifierMappingReply rep;
    register xReq *req;
    unsigned int nbytes;
    XModifierKeymap *res;

    LockDisplay(dpy);
    GetEmptyReq(GetModifierMapping, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xFalse);

    nbytes = (long)rep.length << 2;
    res = (XModifierKeymap *) Xmalloc(sizeof (XModifierKeymap));
    res->modifiermap = (KeyCode *) Xmalloc (nbytes);
    _XReadPad(dpy, res->modifiermap, nbytes);
    res->max_keypermod = rep.numKeyPerModifier;

    UnlockDisplay(dpy);
    SyncHandle();
    return (res);
}

/*
 *	Returns:
 *	0	Success
 *	1	Busy - one or more old or new modifiers are down
 *	2	Failed - one or more new modifiers unacceptable
 */
int
XSetModifierMapping(dpy, modifier_map)
    register Display *dpy;
    register XModifierKeymap *modifier_map;
{
    register xSetModifierMappingReq *req;
    xSetModifierMappingReply rep;
    int         mapSize = modifier_map->max_keypermod << 3;	/* 8 modifiers */

    LockDisplay(dpy);
    GetReqExtra(SetModifierMapping, mapSize, req);

    req->numKeyPerModifier = modifier_map->max_keypermod;

    bcopy(modifier_map->modifiermap, (char *)&req[1], mapSize);

    (void) _XReply(dpy, (xReply *) & rep,
	(sizeof(xSetModifierMappingReply) - sizeof(xReply)) >> 2, xTrue);
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.success);
}

XModifierKeymap *
XNewModifiermap(keyspermodifier)
    int keyspermodifier;
{
    XModifierKeymap *res = (XModifierKeymap *) Xmalloc((sizeof (XModifierKeymap)));

    res->max_keypermod = keyspermodifier;
    res->modifiermap = (keyspermodifier > 0 ?
			(KeyCode *) Xmalloc(8 * keyspermodifier)
			: (KeyCode *) NULL);
    return (res);
}

void
XFreeModifiermap(map)
    XModifierKeymap *map;
{
    if (map) {
	if (map->modifiermap)
	    Xfree((char *) map->modifiermap);
	Xfree((char *) map);
    }
}

XModifierKeymap *
XInsertModifiermapEntry(map, keysym, modifier)
    XModifierKeymap *map;
    KeyCode keysym;
    int modifier;
{
    XModifierKeymap *newmap;
    int i,
	row = modifier * map->max_keypermod,
	newrow,
	lastrow;

    for (i=0; i<map->max_keypermod; i++) {
        if (map->modifiermap[ row+i ] == keysym)
	    return(map); /* already in the map */
        if (map->modifiermap[ row+i ] == 0) {
            map->modifiermap[ row+i ] = keysym;
	    return(map); /* we added it without stretching the map */
	}
    }   

    /* stretch the map */
    newmap = XNewModifiermap(map->max_keypermod+1);
    newrow = row = 0;
    lastrow = map->max_keypermod * 8;
    while (row < lastrow) {
	for (i=0; i<map->max_keypermod; i++)
	    newmap->modifiermap[ newrow+i ] = map->modifiermap[ row+i ];
	newmap->modifiermap[ newrow+i ] = 0;
	row += map->max_keypermod;
	newrow += newmap->max_keypermod;
    }
    XFreeModifiermap(map);
    newrow = newmap->max_keypermod * modifier + newmap->max_keypermod - 1;
    newmap->modifiermap[ newrow ] = keysym;
    return(newmap);
}

XModifierKeymap *
XDeleteModifiermapEntry(map, keysym, modifier)
    XModifierKeymap *map;
    KeyCode keysym;
    int modifier;
{
    int i,
	row = modifier * map->max_keypermod;

    for (i=0; i<map->max_keypermod; i++) {
        if (map->modifiermap[ row+i ] == keysym)
            map->modifiermap[ row+i ] = 0;
    }
    /* should we shrink the map?? */
    return (map);
}
