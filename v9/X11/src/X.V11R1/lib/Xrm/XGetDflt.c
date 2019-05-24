#include <copyright.h>

/* $Header: XGetDflt.c,v 1.1 87/08/22 10:24:06 ham Exp $ */
/* Copyright (c) 1985, Massachusetts Institute of Technology */

/*
 * This routine returns options out of the X user preferences file
 * found in the RESOURCE_MANAGER property, possibly modified by the 
 * .Xdefaults in the user's home
 * directory.  It either returns a pointer to
 * the option or returns NULL if option not set.  It is patterned after
 * Andrew's file format (why be different for the sake of being different?).
 * Andrew's code was NOT examined before writing this routine.
 * It parses lines of the format "progname.option:value" and returns a pointer
 * to value.
 */

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include "Xlibint.h"
#include "Xresource.h"
#include "Quarks.h"

#define XOPTIONFILE "/.Xdefaults"	/* name in home directory of options
					   file. */

static XrmResourceDataBase InitDefaults (dpy)
    Display *dpy;			/* display for defaults.... */
{
    XrmResourceDataBase userdb = NULL;
    XrmResourceDataBase xdb = NULL;
    char fname[BUFSIZ];             /* longer than any conceivable size */
    char *getenv();
    char *home = getenv("HOME");

    /*
     * attempt to generate user's file name
     */
    fname[0] = '\0';
    if (home != NULL) {
	strcpy (fname, home);
	strcat (fname, XOPTIONFILE);
	}
    XrmInitialize();

    if (fname[0] != '\0') userdb =  XrmGetDataBase(fname);
    xdb = XrmLoadDataBase(dpy->xdefaults);
    XrmMergeDataBases(userdb, &xdb);
    return xdb;
}

char *XGetDefault(dpy, prog, name)
	Display *dpy;			/* display for defaults.... */
	register char *name;		/* name of option program wants */
	char *prog;			/* name of program for option	*/

{					/* to get, for example, "font"  */
	char temp[BUFSIZ];
	XrmName namelist[5];
	XrmClass classlist[5];
	XrmValue result;

	/*
	 * see if database has ever been initialized.  Lookups can be done
	 * without locks held.
	 */
	LockDisplay(dpy);
	if (dpy->db == NULL) {
		dpy->db = InitDefaults(dpy);
		}
	UnlockDisplay(dpy);
	sprintf(temp, "%s.%s", prog, name);
	XrmStringToNameList(temp, namelist);
	XrmStringToClassList("Program.Name", classlist);

	XrmGetResource(DefaultScreen(dpy), dpy->db, 
		namelist, classlist, XrmQString, &result);
	return (result.addr);
}

