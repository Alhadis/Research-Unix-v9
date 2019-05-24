#include "copyright.h"

/* $Header: XFreeEData.c,v 11.11 87/09/11 08:03:40 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

_XFreeExtData (extension)
     XExtData *extension;
{
	XExtData *temp;
	while (extension) {
		if (extension->free_private) 
		    (*extension->free_private)(extension);
		else Xfree ((char *)extension->private_data);
		temp = extension->next;
		Xfree ((char *)extension);
		extension = temp;
	}
	return;
}
