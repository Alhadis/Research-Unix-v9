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
/* $Header: filenames.c,v 1.27 87/09/08 08:50:26 swick Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <ctype.h>
#include <strings.h>
#include "misc.h"
#include "resource.h"
#include "osstruct.h"
#include "X.h"
#include "Xmd.h"
#include "opaque.h"

extern char *defaultFontPath;

/* The fontSearchList should contain zero paths to start with. */
static FontPathRec fontSearchList = { 0, 0, 0};

/*
 * description
 *	DDX interface routines
 *
 * caveats:
 *	These are OS-dependent to the extent that they assume pathnames contain
 *	no NULL characters.
 */


/*
 * This is called from main.c, not in response to a protocol request, so it
 * may take a null-terminated string as argument.
 */

void
SetDefaultFontPath( name)
    char *	name;
{
    int		len = strlen( name);
    int i;
/* If there is a previous fontSearchList, free the space that it contains. */

    for (i=0; i<fontSearchList.npaths; i++)
        Xfree(fontSearchList.paths[i]);
    Xfree(fontSearchList.paths);
    Xfree(fontSearchList.length);

    if (name[len-1] != '/')
        len++;
    fontSearchList.npaths = 1;
    fontSearchList.length = (int *)Xalloc(sizeof(int));
    fontSearchList.length[0] = len;
    fontSearchList.paths = (char **)Xalloc(sizeof(char *));
    fontSearchList.paths[0] = (char *)Xalloc(len + 1);
    bcopy(name, fontSearchList.paths[0], len);
    if (name[strlen(name)-1] != '/')
        fontSearchList.paths[0][len-1] = '/';
    fontSearchList.paths[0][len] = '\0';        
}


void
FreeFontRecord(pFP)
    FontPathPtr pFP;
{
    int i;
    for (i=0; i<pFP->npaths; i++)
        Xfree(pFP->paths[i]);
    Xfree(pFP->paths);
    Xfree(pFP->length);
    Xfree(pFP);
}

/*
 * pnames is a pointer to counted strings, each string long word
 * aligned
 */
void
SetFontPath( npaths, totalLength, countedStrings)
    int		npaths;
    int		totalLength;
    char *	countedStrings;
{
    int i;
    char * bufPtr = countedStrings;
    char n, len;

    if (npaths == 0)
    {
	SetDefaultFontPath(defaultFontPath); /* this frees old paths */
    }
    else
    {
        for (i=0; i<fontSearchList.npaths; i++)
	    Xfree(fontSearchList.paths[i]);
	Xfree(fontSearchList.paths);
	Xfree(fontSearchList.length);

        fontSearchList.length = (int *)Xalloc(npaths * sizeof(int));
        fontSearchList.paths = (char **)Xalloc(npaths * sizeof(char *));
	for (i=0; i<npaths; i++)
        {
	    n = len = *bufPtr;
            if (bufPtr[n] != '/')
                len++;
	    fontSearchList.length[i] = len;
	    fontSearchList.paths[i] = (char *) Xalloc(len + 1);
	    bcopy(bufPtr+1, fontSearchList.paths[i], n);
            if (bufPtr[n] != '/')
                fontSearchList.paths[i][n] = '/';
	    fontSearchList.paths[i][len] = '\0';
	    bufPtr += n + 1;
	}
	fontSearchList.npaths = npaths;
    }
}


/*
 * return value is length in bytes 
 */

FontPathPtr
GetFontPath()
{
    return( & fontSearchList);
}

static int
match( pat, string)
    char	*pat;
    char	*string;
{
    int	ip;
    Bool matched;

    for ( ip=0; pat[ip]!='\0'; ip++)
    {
        if ( pat[ip] == '*')
	{
	    matched = FALSE;
	    while (! matched )   /* find where pat & string start to match */
	    {
		if ( string[ip] == '\0')
		{
		    if (pat[ip+1] == '\0')
		        return TRUE;
		    break;
		}
		while ( ! (matched = match( &pat[ip+1], &string[ip])))
		{
		    string++;
		    if ( string[0] == '\0') 
		        return FALSE;
		}
	    }
            if (matched)
                return TRUE;
	}
        else if (string[ip] == '\0')
            return FALSE;
	else if (( pat[ip] != '?') && (pat[ip] != string[ip]))
	     return FALSE;
    }
    return TRUE;
}


/*
 * Make further assumption that '/' is the pathname separator.
 * Still assume NULL ends strings.
 */
/*
 * used by OpenFont
 *
 *  returns length of ppathname.
 *  may set *ppPathname to fname;
 */

int
ExpandFontName( ppPathname, lenfname, fname)
    char	**ppPathname;
    int		lenfname;
    char	*fname;
{
    int		in;
    char	*fullname = NULL;
    char	*lowername;
    char	*pch;

    lowername = (char *) Xalloc(lenfname + 5);
    bzero(lowername, lenfname + 5);
    strncpy(lowername, fname, lenfname);
    /* if the name doesn't end in .snf, append that to the name */
    if((pch = index(lowername, '.')) == NULL || 
     *(pch + 1) != 's' ||
     *(pch + 2) != 'n' ||
     *(pch + 3) != 'f' ||
     pch + 4 - lowername != lenfname)
    {
	strcat(lowername, ".snf");
	lenfname += 4;
    }
	

    /*
     * reduce to lower case only
     */
    for ( in=0; in<lenfname; in++)
	if ( isupper( lowername[in]))
	    lowername[in] = tolower( lowername[in]);
    
    if (lowername[0] == '/')
    {
	if ( access( lowername, R_OK) == 0)
	{
	    *ppPathname = lowername;
	    return lenfname;
	}
    }
    else	
    {
	int		n, ifp;

	for ( ifp=0; ifp<fontSearchList.npaths; ifp++)
	{
	    fullname = (char *) Xrealloc( fullname, 
			  n = fontSearchList.length[ifp] + lenfname + 1);
	    strcpy( fullname, fontSearchList.paths[ifp]);
	    strncat( fullname, lowername, lenfname);
	    fullname[n-1] = '\0';
	    if ( access( fullname, R_OK) == 0)
	    {
		*ppPathname = fullname;
		Xfree(lowername);
		return strlen( fullname);
	    }
	}
    }
    Xfree(lowername);
    Xfree(fullname);
    *ppPathname = NULL;
    return 0;
}

static void
SearchDirectory(dname, pat, pFP, limit)
    char *dname;
    char *pat;
    FontPathPtr pFP;
    int limit;
{
    DIR		*dp;
    struct direct *nextdent;
    int		n, i;

    dp = opendir( dname);  
    if (dp == NULL)
        return ;
    i = pFP->npaths;
    while ( (nextdent = readdir( dp)) != NULL) 
    { 
   	if (i >= limit)  
	    break ;
	if ( match( pat, nextdent->d_name))  
	{  
	    pFP->length = (int *)Xrealloc(pFP->length, (i+1)*sizeof(int));
	    pFP->paths = (char **)Xrealloc(pFP->paths, (i+1)*sizeof(char *));  
	    pFP->length[i] = n = strlen( nextdent->d_name);  
	    pFP->paths[i] = (char *) Xalloc(n);
	    bcopy(nextdent->d_name, pFP->paths[i], n);
	    i = ++pFP->npaths;
	}
    }
    closedir(dp);
}

/*******************************************************************
 *  ExpandFontPathPattern
 *
 *	Returns a FontPathPtr with at most max-names, of names of fonts
 *      matching
 *	the pattern.  The pattern should use the ASCII encoding, and
 *      upper/lower case does not matter.  In the pattern, the '?' character
 *	(octal value 77) will match any single character, and the character '*'
 *	(octal value 52) will match any number of characters.  The return
 *	names are in lower case.
 *
 *      Used only by protocol request ListFonts
 *******************************************************************/


FontPathPtr
ExpandFontNamePattern(lenpat, countedPattern, maxNames)
    int		lenpat;
    char	*countedPattern;
    int		maxNames;
{
    char	*pattern;
    int		i;
    FontPathPtr	fpr;


    fpr = (FontPathPtr)Xalloc(sizeof(FontPathRec));
    fpr->npaths = 0;
    fpr->length = (int *)NULL;
    fpr->paths = (char **)NULL;

    /*
     * make a pattern which is guaranteed NULL-terminated
     */
    pattern = (char *) ALLOCATE_LOCAL( lenpat + 1 + 4);
    strncpy( pattern, countedPattern, lenpat);
    pattern[lenpat] = '\0';
    strcat(pattern, ".snf");

    /*
     * find last '/' in pattern, if any
     */
    for ( i=lenpat-1; i>=0; i--)
	if ( pattern[i] == '/')
	    break;

    if ( i >= 0)		/* pattern contains its own dir prefix */
    {
	pattern[i] = '\0';	/* break pattern at the last path separator */
        SearchDirectory(pattern, &pattern[i+1], fpr, maxNames);
    }
    else
    {
        /*
	 * for each prefix in the font path list
	 */
	for ( i=0; i<fontSearchList.npaths; i++)
	{
	    SearchDirectory( fontSearchList.paths[i], pattern, fpr, maxNames);
	    if (fpr->npaths >= maxNames)
	        break;
	}
    }
    DEALLOCATE_LOCAL(pattern);
    /*
     * logically strip the ".snf" off the end since the client wants font
     * names and not file names.
     */
    for (i = 0; i < fpr->npaths; i++)
	fpr->length[i] -= 4;
    return fpr;
}


