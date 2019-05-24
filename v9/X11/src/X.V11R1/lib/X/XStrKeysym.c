/* $Header: XStrKeysym.c,v 11.1 87/08/30 21:18:57 jg Exp $ */
/* Copyright 1985, 1987, Massachusetts Institute of Technology */

#include "Xlibint.h"
static struct ks_info {
    char	*ks_name;
    KeySym	ks_val;
} keySymInfo[] = {
#include	"ks_names.h"
};

KeySym XStringToKeysym(s)
    char *s;
{
    int i;

    /*
     *	Yes,  yes,  yes.  I know this is a linear search,  and we should
     *	do better,  but I'm in a hurry right now.
     */

    for (i = 0; i < ((sizeof keySymInfo)/(sizeof keySymInfo[0])); i++) {
	if (strcmp(s, keySymInfo[i].ks_name) == 0)
	    return (keySymInfo[i].ks_val);
    }
    return (NoSymbol);
}

char *XKeysymToString(ks)
    KeySym ks;
{
    int i;

    /*
     *	Yes,  yes,  yes.  I know this is a linear search,  and we should
     *	do better,  but I'm in a hurry right now.
     */

    for (i = 0; i < ((sizeof keySymInfo)/(sizeof keySymInfo[0])); i++) {
	if (ks == keySymInfo[i].ks_val)
	    return (keySymInfo[i].ks_name);
    }
    return ((char *) NULL);
}
