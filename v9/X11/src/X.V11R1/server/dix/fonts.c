/************************************************************************
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

************************************************************************/

/* $Header: fonts.c,v 1.46 87/09/09 13:23:03 rws Exp $ */

#define NEED_REPLIES
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "scrnintstr.h"
#include "resource.h"
#include "osstruct.h"
#include "dix.h"
#include "cursorstr.h"
#include "misc.h"
#include "opaque.h"

static FontPtr 	pFontHead = NullFont;
extern FontPtr 	defaultFont;


/*
 * adding RT_FONT prevents conflict with default cursor font
 */
SetDefaultFont( defaultfontname)
    char *	defaultfontname;
{
    FontPtr	pf;

    if ((pf =OpenFont( strlen( defaultfontname), defaultfontname)) == NullFont)
	return FALSE;

    AddResource( FakeClientID(0), RT_FONT, pf, CloseFont, RC_CORE);
    defaultFont = pf;
    return TRUE;
}

/*
 * Check reference count first, load font only if necessary.
 */
FontPtr 
OpenFont( lenfname, pfilename)
    int		lenfname;
    char *	pfilename;
{
    FontPtr 	pfont;
    int		lenpname;
    char *	ppathname;
    FID		pf;
    int		nscr;
    ScreenPtr	pscr;

    FontPtr 	ReadNFont();

    /*
     * call os-dependent code
     */
    if ( (lenpname = ExpandFontName( &ppathname, lenfname, pfilename)) == 0)
    {
#ifdef notdef
	ErrorF( "OpenFont: ExpandFontName failed to find file '%s', (len = %d)\n",
		pfilename, lenfname);
#endif
	return NullFont;
    }
    /*
     * first look up font name in list of opened fonts 
     */
    for (   pfont = pFontHead;
	    pfont != NullFont;
	    pfont = pfont->next)
	if ( lenpname == pfont->lenpname
	  && strncmp( pfont->pathname, ppathname, pfont->lenpname) == 0)
	{
	    /*
	     * found it!
	     */
	    pfont->refcnt += 1;
	    Xfree(ppathname);
	    return pfont;
	}
	
    /*
     * if not found in fonts list, read it off disk
     */

    if ( (pf = FiOpenForRead( lenpname, ppathname)) == NullFID)
    {
#ifdef notdef
	ErrorF( 
		"OpenFont: failed to open font file %s\n",
		ppathname);
#endif
	Xfree(ppathname);
	return NullFont;
    }
    pfont = ReadNFont( pf);
    FiClose( pf);

    if ( pfont == NullFont)
    {
#ifdef notdef
	ErrorF(  "OpenFont: ReadNFont failed on file %s\n", ppathname);
#endif
	Xfree(ppathname);
	return NullFont;
    }

    pfont->lenpname = lenpname;
    pfont->pathname = (char *)Xalloc( lenpname);	/* record pathname */
    strncpy( pfont->pathname, ppathname, lenpname);

    pfont->refcnt = 1;
    pfont->next = pFontHead;			/* prepend to fonts list */
    pFontHead = pfont;

    /*
     * since this font has been newly read off disk, ask each screen to
     * realize it.
     */
    for ( nscr=0, pscr=screenInfo.screen;
	  nscr<screenInfo.numScreens;
	  nscr++, pscr++)
    {
        if ( pscr->RealizeFont)
	    ( *pscr->RealizeFont)( pscr, pfont);
    }
    Xfree(ppathname);
    return pfont;
}

/*
 * Decrement font's ref count, and free storage if ref count equals zero
 */
void
CloseFont( pfont)
    FontPtr 	pfont;
{
    FontPtr 	ptf;
    FontPtr 	pbtf = NullFont;
    int		nscr;
    ScreenPtr	pscr;

    if (pfont == NullFont)
        return ;
    for (   ptf = pFontHead;
	    ptf != NullFont;
	    ptf = ptf->next)
    {
	if ( ptf == pfont || ptf == NullFont)
	    break;
	pbtf = ptf;
    }
    if ( ptf == NullFont)
    {
	ErrorF(  "CloseFont: couldn't find font to be closed %x\n", ptf);
	return;
    }
    if ( --ptf->refcnt == 0)
    {
	if ( pbtf == NullFont)		/* unlink ptf */
	    pFontHead = ptf->next;
	else
	    pbtf->next = ptf->next;

	/*
	 * since the last reference is gone, ask each screen to
	 * free any storage it may have allocated locally for it.
	 */
	for ( nscr=0, pscr=screenInfo.screen;
	      nscr<screenInfo.numScreens;
	      nscr++, pscr++)
	{
	    if ( pscr->UnrealizeFont)
		( *pscr->UnrealizeFont)( pscr, ptf);
	}
	Xfree( ptf->pathname);
	Xfree( ptf);
	if (pfont == defaultFont)
	    defaultFont = NULL;
    }
}


/*
 * description
 *
 * Allocates storage with Xalloc(), and returns a pointer to it.
 * Client can call Xfree() to deallocate font.
 *
 * file I/O is all in this routine
 */
FontPtr 
ReadNFont( fp)
    FID		fp;	/* caller has to FiOpenForRead the file himself */
{
    FontPtr 	pfont;
    FontInfoRec	fi;
    DIXFontProp *pdfp;
    FontPropPtr	pffp;
    char	*propspace;
    int		bytestoread, bytestoalloc;
    int		bytesdata;
    int		i;
    char *strings;
	
    /* swap bytes? XXX */

    bytestoread = BYTESOFFONTINFO(&fi);
    if ( FiRead( &fi, 1, bytestoread, fp) != bytestoread)
    {
	ErrorF("ReadNFont: unexpected EOF\n");
        return NullFont;
    }

    if (fi.version1 != FONT_FILE_VERSION || fi.version2 != FONT_FILE_VERSION)
    {
	ErrorF("ReadNFont: bad font version; expected %d, got %d and %d\n",
		FONT_FILE_VERSION, fi.version1, fi.version2);
	return NullFont;
    }

    bytesdata = BYTESOFFONTINFO(&fi);
    bytesdata += BYTESOFCHARINFO(&fi);
    bytesdata += BYTESOFGLYPHINFO(&fi);

    bytestoalloc = BYTESOFFONTINFO(&fi)+bytesdata+BYTESOFPROPINFO(&fi);
    pfont = (FontPtr ) Xalloc(bytestoalloc);

    bytestoread = bytesdata - BYTESOFFONTINFO(&fi);
    if ( FiRead((char *)(&pfont[1]) + BYTESOFFONTINFO(&fi),
		1, bytestoread,
		fp ) != bytestoread)
    {
	ErrorF("ReadNFont: unexpected EOF\n");
	Xfree(pfont);
        return NullFont;
    }

    /*
     * now fix up pointers
     */
    pfont->pFI = (FontInfoPtr)&pfont[1];
    *pfont->pFI = fi;	/* copy date previously read */
    
    pfont->pCI = ADDRCharInfoRec(pfont->pFI);

    pfont->pGlyphs = ADDRCHARGLYPHS(pfont->pFI);

    pfont->pFP = ADDRXFONTPROPS(pfont->pFI);

    /* now read and atom'ize properties */

    bytestoalloc = BYTESOFPROPINFO(pfont->pFI) + BYTESOFSTRINGINFO(pfont->pFI);
    propspace = (char *) Xalloc(bytestoalloc);
    
    pffp = (FontPropPtr)propspace;
    strings = propspace + BYTESOFPROPINFO(pfont->pFI);
    
    bytestoread = bytestoalloc;
    if ( FiRead(
		pffp, 1,
		bytestoread,
		fp) != bytestoread)
    {
	ErrorF("ReadNFont: unexpected EOF\n");
	Xfree(pfont);
	Xfree(propspace);
        return NullFont;
    }

    for (i=0, pdfp=pfont->pFP; i<fi.nProps; i++, pdfp++, pffp++)
    {
	pdfp->name = MakeAtom(
		&strings[pffp->name], strlen(&strings[pffp->name]), 1);
	if (pffp->indirect)
		pdfp->value = (INT32)MakeAtom(
		    &strings[pffp->value], strlen(&strings[pffp->value]), 1);
	else
		pdfp->value = pffp->value;
    }

    Xfree(propspace);

    return pfont;
}

Bool
DescribeFont(pfontname, lenfname, pfi, ppfp)
    char *pfontname;
    int lenfname;
    FontInfoPtr pfi;
    DIXFontPropPtr *ppfp;	/* return */
{
    FontPtr pfont;
    FID pf;
    int i;
    char *ppathname;
    int lenpname;
    int bytesskip, bytesprops;
    DIXFontPropPtr pdfp;
    FontPropPtr pffp, temp;
    char *strings;

    if ((lenpname = ExpandFontName(&ppathname, lenfname, pfontname)) == 0)
        return(FALSE);
    /*
     * first look up font name in list of opened fonts 
     */
    for (   pfont = pFontHead;
	    pfont != NullFont;
	    pfont = pfont->next)
	if ( lenpname == pfont->lenpname
	  && strncmp( pfont->pathname, ppathname, pfont->lenpname) == 0)
	{
	    *pfi = *pfont->pFI;
	    if (pfi->nProps > 0)
	    {
		*ppfp = (DIXFontPropPtr)Xalloc(sizeof(DIXFontProp)*pfi->nProps);
		if (*ppfp == NullDIXFontProp)
		    return(FALSE);
		bcopy((char *)pfont->pFP, (char *)*ppfp,
		    sizeof(DIXFontProp)*pfi->nProps);
	    }
	    return(TRUE);
	}

    /* have to read it off disk */
    if ((pf = FiOpenForRead(lenpname, ppathname)) == NullFID)
	return(FALSE);
    if ((FiRead(pfi, 1, sizeof(FontInfoRec), pf) != sizeof(FontInfoRec)) ||
	pfi->version1 != FONT_FILE_VERSION ||
	pfi->version2 != FONT_FILE_VERSION)
    {
        FiClose(pf);
        return(FALSE);
    }

    bytesskip = BYTESOFFONTINFO(pfi) - sizeof(FontInfoRec) + 
	        BYTESOFCHARINFO(pfi) + BYTESOFGLYPHINFO(pfi);
    bytesprops = BYTESOFPROPINFO(pfi) + BYTESOFSTRINGINFO(pfi);

    temp = (FontPropPtr)Xalloc(max(bytesskip, bytesprops));
    *ppfp = (DIXFontProp *)Xalloc(pfi->nProps * sizeof(DIXFontProp));

    if ((!temp) ||
	(*ppfp == NullDIXFontProp) ||
	(FiRead((char *)temp, 1, bytesskip, pf) != bytesskip) ||
	(FiRead((char *)temp, 1, bytesprops, pf) != bytesprops))
    {
	Xfree((char *)*ppfp);
	*ppfp = NullDIXFontProp;
	Xfree((char *)temp);
        FiClose(pf);
        return(FALSE);
    }
    strings = (char *)&temp[pfi->nProps];
    pffp = temp;
    for (i=0, pdfp=(*ppfp); i<pfi->nProps; i++, pdfp++, pffp++)
    {
	pdfp->name = MakeAtom(
		&strings[pffp->name], strlen(&strings[pffp->name]), 1);
	if (pffp->indirect)
		pdfp->value = (INT32)MakeAtom(
		    &strings[pffp->value], strlen(&strings[pffp->value]), 1);
	else
		pdfp->value = pffp->value;
    }
    Xfree((char *)temp);
    FiClose(pf);
    return(TRUE);
}


#ifdef notdef
/*
 * assumes short is 2 bytes and long is 4, but nothing about padding
 */
#define _us	unsigned short
#define _ul	unsigned long
#define sw2( p)		\
	*((_us *)p) =	\
	      *(_us *)p>>8	\
	    | *(_us *)p<<8;
#define sw4( p)		\
	*((_ul *)p) =	\
	      *(_ul *)p>>24	\
	    | *(_ul *)p>>8 & 0x0000ff00	\
	    | *(_ul *)p<<8 & 0x00ff0000	\
	    | *(_ul *)p<<24;

#else

#define sw2( p) {	\
		int	t;	\
		t = ((char *)p)[0];	\
		((char *)p)[0] = ((char *)p)[1];	\
		((char *)p)[1] = t;	\
	}
#define sw4( p)	{	\
		int	t;	\
		t = ((char *)p)[0];	\
		((char *)p)[0] = ((char *)p)[3];	\
		((char *)p)[3] = t;	\
		t = ((char *)p)[1];	\
		((char *)p)[1] = ((char *)p)[2];	\
		((char *)p)[2] = t;	\
	}

#endif

void
QueryFont( pf, pr, nprotoxcistructs)
    FontPtr 		pf;
    xQueryFontReply *	pr;	/* caller must allocate this storage */
    int		nprotoxcistructs;
{
    FontInfoPtr 	pfi = pf->pFI;
    CharInfoPtr 	pci;
    DIXFontProp *	pfp;
    int		ct;
    xFontProp *	prfp;
    xCharInfo *	prci;

    void	queryCharInfo();

    /* pr->length set in dispatch */
    pr->minCharOrByte2 = pfi->firstCol;
    pr->defaultChar = pfi->chDefault;
    pr->maxCharOrByte2 = pfi->lastCol;
    pr->drawDirection = pfi->drawDirection;
    pr->allCharsExist = pfi->allExist;
    pr->minByte1 = pfi->firstRow;
    pr->maxByte1 = pfi->lastRow;
    pr->fontAscent = pfi->fontAscent;
    pr->fontDescent = pfi->fontDescent;

    queryCharInfo( &pfi->minbounds, &pr->minBounds); 
    queryCharInfo( &pfi->maxbounds, &pr->maxBounds); 

    pr->nFontProps = pfi->nProps; 
    pr->nCharInfos = nprotoxcistructs; 


    for ( ct=0,
	    pfp=pf->pFP,
	    prfp=(xFontProp *)(&pr[1]);
	  ct < pfi->nProps;
	  ct++, pfp++, prfp++)
    {
	prfp->name = pfp->name;
	prfp->value = pfp->value;
    }

    for ( ct=0,
	    pci = &pf->pCI[0],
	    prci=(xCharInfo *)(prfp);
	  ct<nprotoxcistructs;
	  ct++, pci++, prci++)
	queryCharInfo( pci, prci);
}

/* static */ void
queryCharInfo( pci, pr)
    CharInfoPtr 		pci;
    xCharInfo *		pr;	/* protocol packet to fill in */
{
    *pr = pci->metrics;
}

/* static */ void
SwapFont( pr, hasGlyphs)
    xQueryFontReply *	pr;
    Bool hasGlyphs;
{
    int		i;
    xCharInfo *	pxci;
    int		nchars, nprops;
    char	*pby;
    register char n;

    swaps(&pr->sequenceNumber, n);
    swapl(&pr->length, n);
    nchars = pr->nCharInfos;
    nprops = pr->nFontProps;
    SwapFontInfo(pr);
    pby = (char *) &pr[1];
    /* Font properties are an atom and either an int32 or a CARD32, so
     * they are always 2 4 byte values */
    for(i = 0; i < nprops; i++)
    {
	swapl(pby, n);
	pby += 4;
	swapl(pby, n);
	pby += 4;
    }
    if (hasGlyphs)
    {
	pxci = (xCharInfo *)pby;
	for(i = 0; i< nchars; i++, pxci++)
	    SwapCharInfo(pxci);
    }
}

SwapFontInfo(pr)
    xQueryFontReply *pr;
{
    register char		n;

    swaps(&pr->minCharOrByte2, n);
    swaps(&pr->maxCharOrByte2, n);
    swaps(&pr->defaultChar, n);
    swaps(&pr->nFontProps, n);
    swaps(&pr->fontAscent, n);
    swaps(&pr->fontDescent, n);
    SwapCharInfo( &pr->minBounds);
    SwapCharInfo( &pr->maxBounds);
    swapl(&pr->nCharInfos, n);
}
SwapCharInfo(pInfo)
    xCharInfo	*pInfo;
{
    register char n;

    swaps(&pInfo->leftSideBearing, n);
    swaps(&pInfo->rightSideBearing, n);
    swaps(&pInfo->characterWidth, n);
    swaps(&pInfo->ascent, n);
    swaps(&pInfo->descent, n);
    swaps(&pInfo->attributes, n);
}


/* text support routines. A charinfo array builder, and a bounding */
/* box calculator */

void
GetGlyphs(font, count, chars, fontEncoding, glyphcount, glyphs)
    FontPtr font;
    int count;
    register unsigned char *chars;
    FontEncoding fontEncoding;
    unsigned int *glyphcount;	/* RETURN */
    CharInfoPtr glyphs[];	/* RETURN */
{
    CharInfoPtr		pCI = font->pCI;
    FontInfoPtr		pFI = font->pFI;
    unsigned int	firstCol = pFI->firstCol;
    unsigned int	numCols = pFI->lastCol - firstCol + 1;
    unsigned int	firstRow = pFI->firstRow;
    unsigned int	numRows = pFI->lastRow - firstRow + 1;
    unsigned int	chDefault = pFI->chDefault;
    register int	i;
    int			n;
    register unsigned int	c;
    register CharInfoPtr	ci;

    n = 0;
    switch (fontEncoding) {

	case Linear8Bit:
	case TwoD8Bit:
	    for (i=0; i < count; i++) {

		c = (*chars++) - firstCol;
		if (c < numCols) {
		    ci = &pCI[c];
		    if (ci->exists) {glyphs[n++] = ci; continue;}
		}

		c = chDefault - firstCol;
		if (c < numCols) {
		    ci = &pCI[c];
		    if (ci->exists) glyphs[n++] = ci;
		}
	    }
	    break;

	case Linear16Bit:
	    for (i=0; i < count; i++) {

		chars++;
		c = (*chars++) - firstCol;
		if (c < numCols) {
		    ci = &pCI[c];
		    if (ci->exists) {glyphs[n++] = ci; continue;}
		}

		c = chDefault - firstCol;
		if (c < numCols) {
		    ci = &pCI[c];
		    if (ci->exists) glyphs[n++] = ci;
		}
	    }
	    break;

	case TwoD16Bit:
	    for (i=0; i < count; i++) {
		register unsigned int row;
		register unsigned int col;

		row = (*chars++) - firstRow;
		col = (*chars++) - firstCol;
		if ((row < numRows) && (col < numCols)) {
		    c = row*numCols + col;
		    ci = &pCI[c];
		    if (ci->exists) {glyphs[n++] = ci; continue;}
		}

		row = (chDefault >> 8)-firstRow;
		col = (chDefault & 0xff)-firstCol;
		if ((row < numRows) && (col < numCols)) {
		    c = row*numCols + col;
		    ci = &pCI[c];
		    if (ci->exists) glyphs[n++] = ci;
		}
	    }
	    break;
    }
    *glyphcount = n;
}



void
QueryGlyphExtents(font, charinfo, count, info)
    FontPtr font;
    CharInfoPtr *charinfo;
    unsigned int count;
    ExtentInfoRec *info;
{
    int	i;

    info->drawDirection = font->pFI->drawDirection;

    info->fontAscent = font->pFI->fontAscent;
    info->fontDescent = font->pFI->fontDescent;

    if (count != 0) {

	info->overallAscent  = charinfo[0]->metrics.ascent;
	info->overallDescent = charinfo[0]->metrics.descent;
	info->overallLeft    = charinfo[0]->metrics.leftSideBearing;
	info->overallRight   = charinfo[0]->metrics.rightSideBearing;
	info->overallWidth   = charinfo[0]->metrics.characterWidth;

	for (i=1; i < count; i++) {
	    info->overallAscent = max(
	        info->overallAscent,
		charinfo[i]->metrics.ascent);
	    info->overallDescent = max(
	        info->overallDescent,
		charinfo[i]->metrics.descent);
	    info->overallLeft = min(
		info->overallLeft,
		info->overallWidth+charinfo[i]->metrics.leftSideBearing);
	    info->overallRight = max(
		info->overallRight,
		info->overallWidth+charinfo[i]->metrics.rightSideBearing);
	    /* yes, this order is correct; overallWidth IS incremented last */
	    info->overallWidth += charinfo[i]->metrics.characterWidth;
	}

    } else {

	info->overallAscent  = 0;
	info->overallDescent = 0;
	info->overallWidth   = 0;
	info->overallLeft    = 0;
	info->overallRight   = 0;

    }
}

void
QueryTextExtents(font, count, chars, info)
    FontPtr font;
    unsigned int count;
    unsigned short *chars;
    ExtentInfoRec *info;
{
    CharInfoPtr *charinfo =
	(CharInfoPtr *)ALLOCATE_LOCAL(count*sizeof(CharInfoPtr));
    unsigned int n;

    if(!charinfo)
	return;
    if (font->pFI->lastRow == 0)
	GetGlyphs(font, count, chars, Linear16Bit, &n, charinfo);
    else
	GetGlyphs(font, count, chars, TwoD16Bit, &n, charinfo);

    QueryGlyphExtents(font, charinfo, n, info);

    DEALLOCATE_LOCAL(charinfo);
}

