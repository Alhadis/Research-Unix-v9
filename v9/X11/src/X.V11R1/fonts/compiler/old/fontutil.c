#include <stdio.h>
#include <sys/file.h>

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "misc.h"
#include "fontstruct.h"

#include "port.h"	/* used by converters only */
#include "fc.h"		/* used by converters only */

#ifndef u_char
#define u_char	unsigned char
#endif

extern int	glyphPad;
extern int	bitorder;

char *	malloc();
char *	invertbitsandbytes();

/*
 * Computes accelerators, which by definition can be extracted from
 * the font; therefore a pointer to the font is the only argument.
 *
 * Should also give a final sanity-check to the font, so it can be
 * written to disk.
 *
 * Generates values for the following fields in the FontInfo structure:
 *	version
 *	minbounds
 *	maxbounds	- except byteOffset 
 * Generates values for the following fields in the CharInfo array.
 *	u.flags
 */
computeNaccelerators(font)
    TempFont *font;
{
    int		chi, nchars;
    register CharInfoPtr minbounds = &font->pFI->minbounds;
    register CharInfoPtr maxbounds = &font->pFI->maxbounds;

    int		maxoverlap = MINSHORT;		/* used to set fiFlags */
    int		nnonexistchars = 0;	/* used to set fiFlags */

    font->pFI->version1 = font->pFI->version2 = FONT_FILE_VERSION;

    minbounds->metrics.ascent = MAXSHORT;
    minbounds->metrics.descent = MAXSHORT;
    minbounds->metrics.leftSideBearing = MAXSHORT;
    minbounds->metrics.rightSideBearing = MAXSHORT;
    minbounds->metrics.characterWidth = font->pCI->metrics.characterWidth;
    /* don't touch byteOffset! */
    minbounds->exists = 0;
    minbounds->metrics.attributes = 0xFFFF;	/* all bits on */

    maxbounds->metrics.ascent = MINSHORT;
    maxbounds->metrics.descent = MINSHORT;
    maxbounds->metrics.leftSideBearing = MINSHORT;
    maxbounds->metrics.rightSideBearing = MINSHORT;
    maxbounds->metrics.characterWidth = font->pCI->metrics.characterWidth;
    /* don't touch byteOffset! */
    maxbounds->exists = 0;
    maxbounds->metrics.attributes = 0;

    nchars = n2dChars(font->pFI);
    for (chi = 0; chi < nchars; chi++)
	{
    	register CharInfoPtr pci = &font->pCI[chi];

	if (	pci->metrics.ascent == 0
	     &&	pci->metrics.descent == 0
	     &&	pci->metrics.leftSideBearing == 0
	     &&	pci->metrics.rightSideBearing == 0
	     &&	pci->metrics.characterWidth == 0) {
	    nnonexistchars++;
	    pci->exists = FALSE;
	}
	else {

#define MINMAX(field) \
	if (minbounds->metrics.field > pci->metrics.field) \
	     minbounds->metrics.field = pci->metrics.field; \
	if (maxbounds->metrics.field < pci->metrics.field) \
	     maxbounds->metrics.field = pci->metrics.field;

	    pci->exists = TRUE;
	    MINMAX(ascent);
	    MINMAX(descent);
	    MINMAX(leftSideBearing);
	    MINMAX(rightSideBearing);
	    MINMAX(characterWidth);
	    minbounds->metrics.attributes &= pci->metrics.attributes;
	    maxbounds->metrics.attributes |= pci->metrics.attributes;
	    maxoverlap = MAX(
		maxoverlap,
		pci->metrics.rightSideBearing-pci->metrics.characterWidth);
#undef MINMAX
	}
    }

    if ( maxoverlap <= minbounds->metrics.leftSideBearing)
	font->pFI->noOverlap = TRUE;
    else
	font->pFI->noOverlap = FALSE;

    if ( nnonexistchars == 0)
	font->pFI->allExist = TRUE;
    else
	font->pFI->allExist = FALSE;

    if ( (minbounds->metrics.ascent == maxbounds->metrics.ascent) &&
         (minbounds->metrics.descent == maxbounds->metrics.descent) &&
	 (minbounds->metrics.leftSideBearing ==
		maxbounds->metrics.leftSideBearing) &&
	 (minbounds->metrics.rightSideBearing ==
		maxbounds->metrics.rightSideBearing) &&
	 (minbounds->metrics.characterWidth ==
		maxbounds->metrics.characterWidth) &&
	 (minbounds->metrics.attributes == maxbounds->metrics.attributes)) {
	font->pFI->constantMetrics = TRUE;
	if ( maxbounds->metrics.rightSideBearing + maxbounds->metrics.leftSideBearing ==
	     maxbounds->metrics.characterWidth)
	         font->pFI->terminalFont = TRUE;
    }
    else {
	font->pFI->constantMetrics = FALSE;
	font->pFI->terminalFont = FALSE;
    }
}



/*
 * Note that ReadNFont lives in dix/fontserver.c.  It would seem cleaner if
 * ReadNFont and WriteNFont could live together.
 */
WriteNFont( pfile, font)
    FILE *	pfile;
    TempFont	*font;
{
    int		np;
    FontPropPtr	fp;
    int		off;		/* running index into string table */
    int		strbytes;	/* size of string table */
    char	*strings;	/* string table */
    long	zero = 0;

    /* run through the properties and add up the string lengths */
    strbytes = 0;
    for (fp=font->pFP,np=font->pFI->nProps; np; np--,fp++)
    {
	strbytes += strlen((char *)fp->name)+1;
	if (fp->indirect)
		strbytes += strlen((char *)fp->value)+1;
    }
    font->pFI->lenStrings = strbytes;

    /* build string table and convert pointers to offsets */
    strings = malloc(strbytes);
    off = 0;
    for (fp=font->pFP,np=font->pFI->nProps; np; np--,fp++)
    {
	int l;
	l = strlen(fp->name)+1; /* include null */
	bcopy((char *)fp->name, strings+off, l);
	fp->name = off;
	off += l;
	if (fp->indirect) {
		l = strlen(fp->value)+1; /* include null */
		bcopy((char *)fp->value, strings+off, l);
		fp->value = off;
		off += l;
	}
    }

    fwrite( (char *)font->pFI, BYTESOFFONTINFO(font->pFI), 1, pfile);

    fwrite( (char *)(font->pCI),
	    BYTESOFCHARINFO(font->pFI), 1, pfile);

    fwrite( font->pGlyphs, 1, BYTESOFGLYPHINFO(font->pFI), pfile);

    fwrite( (char *)font->pFP, 1, BYTESOFPROPINFO(font->pFI), pfile);

    fwrite( strings, 1, BYTESOFSTRINGINFO(font->pFI), pfile);
}



DumpFont( pfont, bVerbose)
    TempFont *	pfont;
    int		bVerbose;
{
    FontInfoPtr	pfi = pfont->pFI;
    int bFailure = 0;
    int i;

    if ( pfont == NULL)
    {
	fprintf( stderr, "DumpFont: NULL FONT pointer passed\n");
	exit( 1);
    }
    if ( pfi == NULL)
    {
	fprintf( stderr, "DumpFont: NULL FontInfo pointer passed\n");
	exit( 1);
    }

    printf("version1: 0x%x (version2: 0x%x)\n", pfi->version1, pfi->version2);
    if ( pfi->version1 != FONT_FILE_VERSION)
	printf( "*** NOT CURRENT VERSION ***\n");
    if ( pfi->noOverlap)
	printf( " no overlap");
    if ( pfi->allExist)
	printf( " all exist");
    printf("\nprinting direction: ");
    switch ( pfi->drawDirection)
    {
      case FontLeftToRight:
	printf( "left-to-right\n");
	break;
      case FontRightToLeft:
	printf( "right-to-left\n");
	break;
    }
    printf("first character: 0x%x\n", pfi->chFirst);
    printf("last characters:  0x%x\n", pfi->chLast);

    printf("number of font properties:  0x%x\n", pfi->nProps);
    for (i=0; i<pfi->nProps; i++) {
	printf("  %-15s  ", pfont->pFP[i].name);
	if (pfont->pFP[i].indirect)
		printf("%s\n", pfont->pFP[i].value);
	else
		printf("%d\n", pfont->pFP[i].value);
    }

    printf("default character: 0x%x\n", pfi->chDefault);

    printf("minbounds:\n");
    DumpCharInfo( -1, &pfi->minbounds, 1);
    printf("maxbounds:\n");
    DumpCharInfo( -1, &pfi->maxbounds, 1);
    printf("FontInfo struct: virtual memory address == %x\tsize == %x\n",
	    pfont->pFI, sizeof(FontInfoRec));

    printf("CharInfo array: virtual memory base address == %x\tsize == %x\n",
	    pfont->pCI, n1dChars(pfi)*sizeof(CharInfoRec));

    printf("glyph block: virtual memory address == %x\tsize == %x\n",
	    pfont->pGlyphs, pfi->maxbounds.byteOffset );

    if ( bVerbose>0)
	DumpCharInfo( pfont->pFI->chFirst, pfont->pCI, n1dChars(pfont->pFI));

    if ( bVerbose>1)
    {
        if (bFailure)
            fprintf( stderr, "verbosity not possible with failed read");
        else
/*
	    if ( pixDepth > 1)
		DumpAABitmaps( pfont);
	    else
 */
		DumpBitmaps( pfont);
    }
    printf("\n");
}

DumpCharInfo( first, pxci, count)
    int		first;
    CharInfoPtr	pxci;
    int		count;
{
/*
    printf( "\nrbearing\tlbearing\tdescent\tascent\nwidth\tbyteOffset\tbitOffset\tciFlags");
*/
    if (first >= 0) {
	putchar ('\t');
    }
    printf( "rbearing\tdescent\t\twidth\t\texists\n");
    if (first >= 0) printf("number\t");
    printf( "\tlbearing\tascent\t\tbyteOffset\tciFlags\n");

    while( count--)
    {
	if (first >= 0) printf ("%d\t", first++);
	printf( "%4d\t%4d\t%4d\t%4d\t%4d\t0x%x\t%4s\t0x%x\n",
	    pxci->metrics.rightSideBearing, pxci->metrics.leftSideBearing,
	    pxci->metrics.descent, pxci->metrics.ascent,
	    pxci->metrics.characterWidth, pxci->byteOffset,
	    pxci->exists?"yes":"no", pxci->metrics.attributes);
	pxci++;
    }
}

DumpBitmaps( pFont)
    TempFont *pFont;
{
    int			ch;	/* current character */
    int			r;	/* current row */
    int			b;	/* current bit in the row */
    FontInfoPtr		pFI = pFont->pFI;
    CharInfoPtr		pCI = pFont->pCI;
    u_char *		bitmap = (u_char *)pFont->pGlyphs;
    int			n = n1dChars(pFont->pFI);

    for (ch = 0; ch < n; ch++)
    {
	int bpr = GLWIDTHBYTESPADDED(pCI[ch].metrics.rightSideBearing
		- pCI[ch].metrics.leftSideBearing, glyphPad);
        printf("character %d", ch + pFont->pFI->chFirst);
        if ( !pCI[ch].exists || pCI[ch].metrics.characterWidth == 0) {
	    printf (" doesn't exist\n");
            continue;
	} else {
	    putchar('\n');
	}
        for (r=0; r <  pCI[ch].metrics.descent + pCI[ch].metrics.ascent; r++)
        {
	    unsigned char *row = bitmap + pCI[ch].byteOffset+(r*bpr);
            for ( b=0;
		b < pCI[ch].metrics.rightSideBearing - pCI[ch].metrics.leftSideBearing;
		b++) {
		if (bitorder == LSBFirst) {
			putchar((row[b>>3] & (1<<(b&7)))? '#' : '_');
		} else {
			putchar((row[b>>3] & (1<<(7-(b&7))))? '#' : '_');
		}
	    }
            putchar('\n');
        }
    }
}
