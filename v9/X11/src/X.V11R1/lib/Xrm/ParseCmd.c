/* $Header: ParseCmd.c,v 1.1 87/09/12 12:26:56 toddb Exp $ */
/* $Header: ParseCmd.c,v 1.1 87/09/12 12:26:56 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)ParseCommand.c	1.2	2/25/87";
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


/* XrmParseCommand.c 
   Parse command line and store argument values into resource database */

#include "Xlib.h"
#include "Xlibos.h"
#include "Xresource.h"
#include "Quarks.h"
#include <stdio.h>
#include <sys/types.h>


void XrmParseCommand(rdb, table, tableCount, prependName, argc, argv)
    XrmResourceDataBase *rdb;		/* data base */
    XrmOptionDescList	table;	 	/* pointer to table of valid options */
    int			tableCount;	/* number of options		     */
    XrmAtom		prependName;	/* application name to prepend 	     */
    int			*argc;		/* address of argument count 	     */
    char		**argv;		/* argument list (command line)	     */
{
    Bool 		foundOption;
    static XrmQuark 	fullName[100];
    XrmQuark		*resourceName;
    char		**argsave;
    XrmValue	 	resourceValue;
    int 		i, j, myargc;

    myargc = (*argc); 
    argsave = ++argv;
    if (prependName == NULLATOM) {
        resourceName = fullName;
    } else {
    	fullName[0] = XrmAtomToQuark(prependName);
	resourceName = &fullName[1];
    }

    for (--myargc; myargc>0;--myargc, ++argv) {
	foundOption = False;
	for (i=0; (!foundOption) && i < tableCount; ++i) {
	    foundOption = True;
	    for (j =0; table[i].option[j] != NULL; ++j) {
		if (table[i].option[j] != (*argv)[j]) {
		    foundOption = False;
		    break;
		}
	    }

	    if (foundOption
	     && ((table[i].argKind == XrmoptionStickyArg)
	         || (table[i].argKind == XrmoptionIsArg)
		 || ((*argv)[j] == NULL))) {
		switch (table[i].argKind){
		case XrmoptionNoArg:
		    resourceValue.addr = table[i].value;
		    --(*argc);
		    resourceValue.size = strlen(resourceValue.addr)+1;
		    XrmStringToQuarkList(table[i].resourceName, resourceName);
		    XrmPutResource(rdb, fullName, XrmQString, &resourceValue);
		    break;
			    
		case XrmoptionIsArg:
		    resourceValue.addr = (caddr_t)((*argv));
		    --(*argc);
		    resourceValue.size = strlen(resourceValue.addr)+1;
		    XrmStringToQuarkList(table[i].resourceName, resourceName);
		    XrmPutResource(rdb, fullName, XrmQString, &resourceValue);
		    break;

		case XrmoptionStickyArg:
		    resourceValue.addr = (caddr_t)((*argv)+j);
		    --(*argc);
		    resourceValue.size = strlen(resourceValue.addr)+1;
		    XrmStringToQuarkList(table[i].resourceName, resourceName);
		    XrmPutResource(rdb, fullName, XrmQString, &resourceValue);
		    break;

		case XrmoptionSepArg:
		    ++argv; --myargc; --(*argc); --(*argc);
		    resourceValue.addr = (caddr_t)(*argv);
		    resourceValue.size = strlen(resourceValue.addr)+1;
		    XrmStringToQuarkList(table[i].resourceName, resourceName);
		    XrmPutResource(rdb, fullName, XrmQString, &resourceValue);
		    break;
		
		case XrmoptionSkipArg:
		    --myargc;
		    (*argsave++) = (*argv++);
		    (*argsave++) = (*argv); 
		    break;

		case XrmoptionSkipLine:
		    for (;myargc>0;myargc--)
			(*argsave++) = (*argv++);
		    break;

		}
	    } else foundOption = False;

	}
	
	if (!foundOption) 
	    (*argsave++) = (*argv);  /*compress arglist*/ 
    }

    (*argsave)=NULL; /* put NULL terminator on compressed argv */
}
