#ifndef lint
static char rcs_id[] = "$Header: xrdb.c,v 11.5 87/09/11 19:57:36 jg Exp $";
#endif

/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documentation,
 * and that the name of Digital Equipment Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific, 
 * written prior permission.
 */

/*
 * this program is used to load, or dump the resource manager database
 * in the server.
 *
 * Author: Jim Gettys, August 28, 1987
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <strings.h>
#include <X11/Xatom.h>
#include <ctype.h>

#define MAXRDB 100000
#define DBOK 0

char *SanityCheck();

main (argc, argv)
    int argc;
    char **argv;
{
    Display * dpy;
    int i;
    char *displayname = NULL;
    char *filename = NULL;
    FILE *fp;
    int nbytes;
    char *bptr, *endptr;
    char buffer[MAXRDB];
    int printit = 0;

    for (i = 1; i < argc; i++) {
	if (index (argv[i], ':') != NULL) displayname = argv[i];
	else filename = argv[i];
	if (strcmp ("-q", argv[i]) == NULL) printit = 1;
    }

    /* Open display  */
    if (!(dpy = XOpenDisplay (displayname))) {
	(void) fprintf (stderr, "%s: Can't open display '%s'\n",
		argv[0], XDisplayName (displayname));
	exit (1);
    }

    if (printit == 1) {
	/* user wants to print contents */
	if (dpy->xdefaults)
	    fputs(dpy->xdefaults, stdout);
	exit(0);
	}
    else {
	if (filename != NULL) {
		fp = freopen (filename, "r", stdin);
		if (fp == NULL) {
			fprintf(stderr, "%s: can't open file '%s'\n", 
				argv[0], filename);
			exit(1);
			}
		}
	nbytes = fread(buffer, sizeof(char), MAXRDB, stdin);
	if ((bptr = SanityCheck (buffer)) != DBOK) {
		fprintf(stderr, "%s: database fails sanity check \n'%s'\n", 
			argv[0], bptr);
		exit(1);
		}
	XChangeProperty (dpy, RootWindow(dpy, 0), XA_RESOURCE_MANAGER,
		XA_STRING, 8, PropModeReplace, buffer, nbytes);
	}
	XCloseDisplay(dpy);

}

char *getline(buffer, buf)
	register char *buffer;
	register char *buf;
{
	register char c;
	while (*buffer != '\0') {
		c = *buffer++;
		if (c == '\n') {
			*buf = '\0';
			return (buffer);
			}
		if ( ! isspace(c)) *buf++ = c;
	}
	return (NULL);
}

/*
 * does simple sanity check on data base.  Lines can either be
 * commented, be all white space, or must contain ':'.
 */
char *SanityCheck (buffer)
	char *buffer;
{
    static char buf[BUFSIZ];
    register char *s;
    char *b = buffer;
    register char *i;
    while (1) {
	if ((b = getline(b, buf)) == NULL) return (DBOK);
	if (buf[0] == '#' || buf[0] == '\0') continue;
	if ((i = index (buf, ':')) == NULL) return buf;
	}
}
