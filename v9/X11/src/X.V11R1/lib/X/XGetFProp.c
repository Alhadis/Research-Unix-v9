#include "copyright.h"

/* $Header: XGetFProp.c,v 11.5 87/09/11 08:04:01 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

Bool XGetFontProperty (fs, name, valuePtr)
    XFontStruct *fs;
    register Atom name;
    unsigned long *valuePtr;
    {
    /* XXX this is a simple linear search for now.  If the
      protocol is changed to sort the property list, this should
      become a binary search. */
    register XFontProp *prop = fs->properties;
    register XFontProp *last = prop + fs->n_properties;
    while (prop != last) {
	if (prop->name == name) {
	    *valuePtr = prop->card32;
	    return (1);
	    }
	prop++;
	}
    return (0);
    }

	
    

      
