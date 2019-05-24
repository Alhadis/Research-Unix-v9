/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: fileio.c,v 1.8 87/09/11 07:50:51 toddb Exp $ */
#include <stdio.h>
#include "os.h"
#include "osdep.h"

/*
 *   FiOpenForRead, FiRead, FiClose,
 */

FID
FiOpenForRead( lenn, name)
    int		lenn;
    char 	*name;
{
    FID		fid;
    char	*unixfname = name;


    if ( name[lenn-1] != '\0')
    {
	unixfname = (char *) ALLOCATE_LOCAL( lenn+1);
	strncpy( unixfname, name, lenn);
	unixfname[lenn] = '\0';
    }
    else
	unixfname = name;

    fid = (FID)fopen( unixfname, "r");
    if(unixfname != name)
        DEALLOCATE_LOCAL(unixfname);
    return fid;
}


/*
 * returns 0 on error
 */
int
FiRead( buf, itemsize, nitems, fid)
    char	*buf;
    unsigned	itemsize;
    unsigned	nitems;
    FID		fid;
{
    return fread( buf, itemsize, nitems, (FILE *)fid);
}

int
FiClose(fid)
    FID		fid;
{
    return fclose( (FILE *)fid);
}

