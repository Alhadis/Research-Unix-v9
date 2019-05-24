/* $Header: fc.c,v 1.4 87/07/24 14:50:35 toddb Exp $ */

#include <stdio.h>
#include <sys/file.h> 
#include <errno.h> 
/* #include <malloc.h> */	extern char *malloc(), *realloc();

#include "misc.h"
#include "X.h"
#include "Xproto.h"
#include "fontstruct.h"
#include "font.h"

#include "fc.h"	/* used by converters only */

/*
 * DESCRIPTION
 *	pure filter; filename argument only
 *
 * DISCLAIMER
 *	not much thought has been given to error recovery.
 */
#define INDICES 256
#define MAXENCODING 0xFFFF

extern char *gets(), *index();

static char *myname;	/* initialized from argv[0] */
static char *currentfont;

int linenum = 0;	/* for error messages */
int or_glyphPad = 0;	/* override glyphPading?? */
int or_bitorder = 0;	/* override bitorder?? */


int glyphPad = DEFAULTGLPAD;
int bitorder = DEFAULTBITORDER;

/*
 * read the next line and keep a count for error messages
 */
char *
getline(s)
    char *s;
{
    s = gets(s);
    linenum++;
    while (s) {
	int len = strlen(s);
	if (len && s[len-1] == '\015')
	    s[--len] = '\0';
	if ((len==0) || prefix(s, "COMMENT")) {
	    s = gets(s);
	    linenum++;
	} else break;
    }
    return(s);
}

/*
 * malloc and copy a string value. Handle quoted strings.
 */
char *
remember(s)
char *s;
{
    char *p, *pp;

    /* strip leading white space */
    while (*s && (*s == ' ' || *s == '\t'))
	s++;
    if (*s == 0)
    	return "";
    if (*s != '"') {
	pp = s;
	/* no white space in value */
	for (pp=s; *pp; pp++)
	    if (*pp == ' ' || *pp == '\t' || *pp == '\015' || *pp == '\n') {
	        *pp = 0;
		break;
	    }
	p = malloc(strlen(s)+1);
	strcpy(p, s);
	return p;
    }
    /* quoted string: strip outer quotes and undouble inner quotes */
    s++;
    pp = p = malloc(strlen(s)+1);
    while (*s) {
	if (*s == '"') {
	    if (*(s+1) != '"') {
	    	*p++ = 0;
		return pp;
	    } else {
		s++;
	    }
	}
	*p++ = *s++;
    }
    *p++ = 0; /* just in case; space for it allocated above */
    return pp;
}

/*
 *	Invert bit order within each BYTE of an array.
 *	"In" and "out" may point to the same array.
 */
void
bitorderinvert( in, out, nbytes)
    register unsigned char	*in;
    register unsigned char	*out;
    register int	nbytes;
{
    static int BOTableInitialized = 0;
    static unsigned char BOTable[256];


    if ( !BOTableInitialized)
    {
	int	btabi;
	int	biti;

	for ( btabi=0; btabi<256; btabi++)
	    for ( biti=0; biti<8; biti++)
		if (btabi & 1<<biti)
		    BOTable[btabi] |= (unsigned)0x80 >> biti;
	BOTableInitialized++;
    }
    while ( nbytes--)
	*out++ = BOTable[ *in++];

}

/*
 * return TRUE if str is a prefix of buf
 */
prefix(buf, str)
    char *buf, *str;
{
    return strncmp(buf, str, strlen(str))? FALSE : TRUE;
}

/*
 * return TRUE if strings are equal
 */
streq(a, b)
    char *a, *b;
{
    return strcmp(a, b)? FALSE : TRUE;
}

/*
 * make a byte from the first two hex characters in s
 */
unsigned char
hexbyte(s)
    char *s;
{
    unsigned char b = 0;
    register char c;
    int i;

    for (i=2; i; i--) {
	c = *s++;
	if ((c >= '0') && (c <= '9'))
	    b = (b<<4) + (c - '0');
	else if ((c >= 'A') && (c <= 'F'))
	    b = (b<<4) + 10 + (c - 'A');
	else if ((c >= 'a') && (c <= 'f'))
	    b = (b<<4) + 10 + (c - 'a');
	else
	    return 0; /* bad data */
    } 
    return b;
}

/*
 * fatal error. never returns.
 */
fatal(msg, p1, p2, p3, p4)
    char *msg;
{
    fprintf(stderr, "%s: %s:", myname, currentfont);
    fprintf(stderr, msg, p1, p2, p3, p4);
    if (linenum)
	fprintf(stderr, " at line %d\n", linenum);
    else
	fprintf(stderr, "\n");
    exit(1);
}

/*
 * these properties will be generated if not already present.
 */
#define NULLPROP (FontPropPtr)0;

FontPropPtr pointSizeProp = NULLPROP;
FontPropPtr familyProp = NULLPROP;
FontPropPtr resolutionProp = NULLPROP;
FontPropPtr xHeightProp = NULLPROP;
FontPropPtr weightProp = NULLPROP;
FontPropPtr quadWidthProp = NULLPROP;
#define GENPROPS 6

BOOL haveFontAscent = FALSE;
BOOL haveFontDescent = FALSE;

/*
 * check for known property values
 */

int
specialproperty(pfp, pfi)
    FontPropPtr pfp;
    FontInfoPtr pfi;
{
    if (streq(pfp->name, "FONT_ASCENT") && !pfp->indirect)
    {
	pfi->fontAscent = pfp->value;
	haveFontAscent = TRUE;
	return 0;
    }
    else if (streq(pfp->name, "FONT_DESCENT") && !pfp->indirect)
    {
	pfi->fontDescent = pfp->value;
	haveFontDescent = TRUE;
	return 0;
    }
    else if (streq(pfp->name, "DEFAULT_CHAR") && !pfp->indirect)
    {
	pfi->chDefault = pfp->value;
	return 0;
    }
    else if (streq(pfp->name , "POINT_SIZE"))
	pointSizeProp = pfp;
    else if (streq(pfp->name , "FAMILY_NAME"))
	familyProp = pfp;
    else if (streq(pfp->name , "RESOLUTION"))
	resolutionProp = pfp;
    else if (streq(pfp->name , "X_HEIGHT"))
	xHeightProp = pfp;
    else if (streq(pfp->name , "WEIGHT"))
	weightProp = pfp;
    else if (streq(pfp->name , "QUAD_WIDTH"))
	quadWidthProp = pfp;
    return 1;
}

computeweight(font)
    TempFont *font;
{
    int i;
    int width = 0, area, bits = 0;
    register b;
    register unsigned char *p;

    for (i=0; i<n1dChars(font->pFI); i++)
	width += font->pCI[i].metrics.characterWidth;
    area = width*(font->pFI->fontAscent+font->pFI->fontDescent);
    for (i=0,p=font->pGlyphs; i<font->pFI->maxbounds.byteOffset; i++,p++)
    	for (b=(*p); b; b >>= 1)
	    bits += b & 1;
    return (int)((bits*1000.0)/area);
}

main(argc, argv)
    int		argc;
    char *	argv[];
{
    TempFont	font;
    FontInfoRec	fi;
    CharInfoPtr	cinfos[INDICES];	/* rows waiting to be allocated */
    int		bytesGlAlloced = 1024;	/* amount now allocated for glyphs
					   (bytes) */
    unsigned char *pGl = (unsigned char *)malloc( bytesGlAlloced);
    int		bytesGlUsed = 0;
    int		nGl = 0;
    int		nchars;
    float	pointSize;
    int		xRes, yRes;
    char	linebuf[BUFSIZ];
    char	namebuf[100];
    char	family[100];
    char	*bdffile = NULL;
    unsigned int attributes;
    int		digitWidths = 0, digitCount = 0, ex = 0;
    int		char_row, char_col;
    int		i;
    CharInfoRec	emptyCharInfo;

    myname = argv[0];
    argc--, argv++;
    while (argc--) {
	if (argv[0][0] == '-') {
	    switch (argv[0][1]) {
	    case 'p':	/* Pad Glyphs to Word boundaries */
		switch (argv[0][2]) {
		default:
		    goto usage;
		case '1':
		case '2':
		case '4':
		case '8':
		    glyphPad = argv[0][2] - '0';
		    break;
		}
		or_glyphPad = 1;
		break;
	    case 'm':
		or_bitorder = 1;
		bitorder = MSBFirst;
		break;

	    case 'l':
		or_bitorder = 1;
		bitorder = LSBFirst;
		break;

	    default:
		fprintf(stderr, "bad flag -%c\n", argv[0][1]);
		break;
	    }
	} else {
	    if (bdffile)
	usage:
		fatal("usage: %s [-p#] [bdf file]", myname);
	    bdffile = argv[0];
	    if (freopen( bdffile, "r", stdin) == NULL)
		fatal("could not open  file %s\n", bdffile);
	}
	argv++;
    }
    emptyCharInfo.metrics.leftSideBearing = 0;
    emptyCharInfo.metrics.rightSideBearing = 0;
    emptyCharInfo.metrics.ascent = 0;
    emptyCharInfo.metrics.descent = 0;
    emptyCharInfo.metrics.characterWidth = 0;
    emptyCharInfo.byteOffset = 0;
    emptyCharInfo.exists = FALSE;
    emptyCharInfo.metrics.attributes = 0;

    for (i = 0; i < INDICES; i++)
	cinfos[i] = (CharInfoPtr)NULL;

    font.pFI = &fi;
    fi.firstRow = INDICES;
    fi.lastRow = 0;
    fi.chFirst = INDICES;
    fi.chLast = 0;
    fi.pixDepth = 1;
    fi.glyphSets = 1;
    fi.chDefault = 0;	/* may be overridden by a property */

    getline(linebuf);

    if ((sscanf(linebuf, "STARTFONT %s", namebuf) != 1) ||
	!streq(namebuf, "2.1"))
	fatal("bad 'STARTFONT'");
    getline(linebuf);

    if (sscanf(linebuf, "FONT %s", family) != 1)
	fatal("bad 'FONT'");
    getline(linebuf);

    if (!prefix(linebuf, "SIZE"))
	fatal("missing 'SIZE'");
    if ((sscanf(linebuf, "SIZE %f%d%d", &pointSize, &xRes, &yRes) != 3))
	fatal("bad 'SIZE'");
    if (xRes != yRes)
        fatal("x and y resolution must be equal");
    getline(linebuf);

    if (!prefix(linebuf, "FONTBOUNDINGBOX"))
	fatal("missing 'FONTBOUNDINGBOX'");
    getline(linebuf);

    if (prefix(linebuf, "STARTPROPERTIES")) {
	int nprops;
	FontPropPtr pfp;

	sscanf(linebuf, "%*s%d", &nprops);
	fi.nProps = nprops;
	pfp = (FontPropPtr)malloc((nprops+GENPROPS) * sizeof(FontPropRec));
	font.pFP = pfp;
	getline(linebuf);
	while((nprops-- > 0) && !prefix(linebuf, "ENDPROPERTIES")) {
	    if (sscanf(linebuf, "%s%d", namebuf, &pfp->value) == 2) {
		/* integer value */
		pfp->indirect = FALSE;
	    } else {
		/* value is (possibly quoted) string */
		pfp->indirect = TRUE;
		pfp->value = (INT32)remember(linebuf+strlen(namebuf)+1);
	    }
	    pfp->name = (CARD32)remember(namebuf);
	    if (specialproperty(pfp, &fi))
	        pfp++;
	    else
		fi.nProps--;
	    getline(linebuf);
	}
	if (!prefix(linebuf, "ENDPROPERTIES"))
	    fatal("missing 'ENDPROPERTIES'");
	if (!haveFontAscent || !haveFontDescent)
	    fatal("must have 'FONT_ASCENT' and 'FONT_DESCENT' properties");
	if (nprops != -1)
	    fatal("%d too few properties", nprops+1);
	if (!familyProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)("FAMILY_NAME");
	    pfp->value = (INT32)family;
	    pfp->indirect = TRUE;
	    familyProp = pfp++;
	}
	if (!pointSizeProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)("POINT_SIZE");
	    pfp->value = (INT32)(pointSize*10.0);
	    pfp->indirect = FALSE;
	    pointSizeProp = pfp++;
	}
	if (!weightProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)("WEIGHT");
	    pfp->value = -1;	/* computed later */
	    pfp->indirect = FALSE;
	    weightProp = pfp++;
	}
	if (!resolutionProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)("RESOLUTION");
	    pfp->value = (INT32)((xRes*100.0)/72.27);
	    pfp->indirect = FALSE;
	    resolutionProp = pfp++;
	}
	if (!xHeightProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)("X_HEIGHT");
	    pfp->value = -1;	/* computed later */
	    pfp->indirect = FALSE;
	    xHeightProp = pfp++;
	}
	if (!quadWidthProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)("QUAD_WIDTH");
	    pfp->value = -1;	/* computed later */
	    pfp->indirect = FALSE;
	    quadWidthProp = pfp++;
	}
    } else { /* no properties */
	fatal("missing 'PROPERTIES'");
    }
    getline(linebuf);

    if (!prefix(linebuf, "CHARS"))
        fatal("missing 'CHARS'");
    sscanf(linebuf, "%*s%d", &nchars);
    getline(linebuf);

    while ((nchars-- > 0) && prefix(linebuf, "STARTCHAR"))  {
        int	t;
	int	ix;	/* counts bytes in a glyph */
	int	wx;	/* x component of width */
	int	bw;	/* bounding-box width */
	int	bh;	/* bounding-box height */
	int	bl;	/* bounding-box left */
	int	bb;	/* bounding-box bottom */
	int	enc, enc2;	/* encoding */
	char	*p;	/* temp pointer into linebuf */
	int	bytesperrow, row;
	char	charName[100];

	if (sscanf(linebuf, "STARTCHAR %s", charName) != 1)
	    fatal("bad character name");

	getline( linebuf);
	if ((t=sscanf(linebuf, "ENCODING %d %d", &enc, &enc2)) < 1)
	    fatal("bad 'ENCODING'");
	if (t == 2 && enc == -1)
	    enc = enc2;
	if (enc == -1) {
	    fprintf(stderr,
	        "character '%s'with encoding = -1 ignored at line %d\n",
		charName, linenum);
	    do {
	    	char *s = getline(linebuf);
		if (!s)
		    fatal("Unexpected EOF");
	    } while (!prefix(linebuf, "ENDCHAR"));
	    getline(linebuf);
	    continue;
	}
	if (enc > MAXENCODING)
	    fatal("character '%s' has encoding(=%d) too large", charName, enc);
	char_row = (enc >> 8) & 0xFF;
	char_col = enc & 0xFF;
	fi.firstRow = MIN(fi.firstRow, char_row);
	fi.lastRow = MAX(fi.lastRow, char_row);
	fi.chFirst = MIN(fi.chFirst, char_col);
	fi.chLast = MAX(fi.chLast, char_col);
	if (!cinfos[char_row])
	{
	    cinfos[char_row] =
		(CharInfoPtr)malloc(sizeof(CharInfoRec)*INDICES);
	    bzero(cinfos[char_row], sizeof(CharInfoRec)*INDICES);
	}

	getline( linebuf);
	if (!prefix(linebuf, "SWIDTH"))
	    fatal("bad 'SWIDTH'");

	getline( linebuf);
	sscanf( linebuf, "DWIDTH %d %*d", &wx);

	getline( linebuf);
	sscanf( linebuf, "BBX %d %d %d %d", &bw, &bh, &bl, &bb);

	getline( linebuf);
	if (prefix(linebuf, "ATTRIBUTES"))
	{
	    for (p = linebuf + strlen("ATTRIBUTES ");
		(*p == ' ') || (*p == '\t');
		p ++)
		/* empty for loop */ ;
	    attributes = hexbyte(p)<< 8 + hexbyte(p+2);
	    getline( linebuf);	/* set up for BITMAP which follows */
	}
	else
	    attributes = 0;
	if (!prefix(linebuf, "BITMAP"))
	    fatal("missing 'BITMAP'");

	/* collect data for generated properties */
	if ((strlen(charName) == 1)){
	    if ((charName[0] >='0') && (charName[0] <= '9')) {
		digitWidths += wx;
		digitCount++;
	    } else if (charName[0] == 'x') {
	        ex = (bh+bb)<=0? bh : bh+bb ;
	    }
	}

	cinfos[char_row][char_col].metrics.leftSideBearing = bl;
	cinfos[char_row][char_col].metrics.rightSideBearing = bl+bw;
	cinfos[char_row][char_col].metrics.ascent = bh+bb;
	cinfos[char_row][char_col].metrics.descent = -bb;
	cinfos[char_row][char_col].metrics.characterWidth = wx;
	cinfos[char_row][char_col].byteOffset = bytesGlUsed;
	cinfos[char_row][char_col].exists = FALSE;  /* overwritten later */
	cinfos[char_row][char_col].metrics.attributes = attributes;

	bytesperrow = GLWIDTHBYTESPADDED(bw,glyphPad);
	for (row=0; row < bh; row++) {
	    getline(linebuf);
	    p = linebuf;
	    for ( ix=0; ix < bytesperrow; ix++)
	    {
	        if ( bytesGlUsed >= bytesGlAlloced)
	    	pGl = (unsigned char *)realloc( pGl, (bytesGlAlloced *= 2));
	        pGl[bytesGlUsed] = hexbyte(p);
	        p += 2;
	        bytesGlUsed++;
	    }
 	    /*
 	     *  Now pad the glyph row our pad boundary.
 	     */
	    bytesGlUsed = GLWIDTHBYTESPADDED(bytesGlUsed<<3,glyphPad);
	}
	getline( linebuf);
	if (!prefix(linebuf, "ENDCHAR"))
            fatal("missing 'ENDCHAR'");
	nGl++;
	getline( linebuf);		/* get STARTCHAR or ENDFONT */
    }

    if (!prefix(linebuf, "ENDFONT"))
        fatal("missing 'ENDFONT'");
    if (nchars != -1)
        fatal("%d too few characters", nchars+1);
    if (nGl == 0)
        fatal("No characters with valid encodings");

    fi.maxbounds.byteOffset = bytesGlUsed;
    font.pGlyphs = pGl;

    font.pCI = (CharInfoPtr)malloc(sizeof(CharInfoRec)*n2dChars(font.pFI));
    i = 0;
    for (char_row = fi.firstRow; char_row <= fi.lastRow; char_row++)
    {
	if (!cinfos[char_row])
	    for (char_col = fi.chFirst; char_col <= fi.chLast; char_col++)
		{
		font.pCI[i] = emptyCharInfo;
		i++;
		}
	else
	    for (char_col = fi.chFirst; char_col <= fi.chLast; char_col++)
		{
		font.pCI[i] = cinfos[char_row][char_col];
		i++;
		}
    }
    computeNaccelerators(&font);

    /* generate properties */
    if (xHeightProp && (xHeightProp->value == -1))
        xHeightProp->value = ex? ex : fi.minbounds.metrics.ascent;
    if (quadWidthProp && (quadWidthProp->value == -1))
        quadWidthProp->value = digitCount?
	    (INT32)((float)digitWidths/(float)digitCount) :
	    (fi.minbounds.metrics.characterWidth+fi.maxbounds.metrics.characterWidth)/2;
    if (weightProp && (weightProp->value == -1))
        weightProp->value = computeweight(&font);

    if (bitorder == LSBFirst)
	bitorderinvert( pGl, pGl, bytesGlUsed);
#ifdef UNSPECIFIED
    if (!or_glyphPad || !or_bitorder) {
	fprintf(stderr, "%s: ", currentfont);
	if (!or_glyphPad && !or_bitorder)
	    fprintf(stderr, "bit order/pad unspecified:\n\t");
	else if (!or_glyphPad)
	    fprintf(stderr, "pad unspecified:\n\t");
	else
	    fprintf(stderr, "bit order unspecified:\n\t");
	fprintf(stderr, "using ");

	if (!or_bitorder)
	    fprintf(stderr, "order=%s",
		bitorder == LSBFirst ? "LSBFirst" : "MSBFirst");
	if (!or_glyphPad && !or_bitorder)
	    fprintf(stderr, ", ");
	if (!or_glyphPad)
	    fprintf(stderr, "pad=%d", glyphPad);
	fprintf(stderr, "\n");
    }
#endif

    WriteNFont( stdout, &font);
    exit(0);
}

