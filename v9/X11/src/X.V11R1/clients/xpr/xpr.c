#include <X11/copyright.h>

/*
 * XPR - process xwd(1) files for the LN03 or LA100 printer
 *
 * Author: Michael R. Gretzinger, MIT Project Athena
 * Copyright (C) 1985, Massachusetts Institute of Technology
 *
 * Modified by Marvin Solomon, Univeristy of Wisconsin, to handle Apple
 * Laserwriter (PostScript) devices (-device ps).
 * Also accepts the -compact flag that produces more compact output
 * by using run-length encoding on white (1) pixels.
 * This version does not (yet) support the following options
 *   -append -dump -noff -nosixopt -split
 * 
 * Changes
 * Copyright 1986 by Marvin Solomon and the University of Wisconsin
 * 
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the names of Marvin Solomon and
 * the University of Wisconsin not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * Neither Marvin Solomon nor the University of Wisconsin
 * makes any representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * 
 */

#ifndef lint
static char *rcsid_xpr_c = "$Header: xpr.c,v 1.18 87/09/11 20:02:55 toddb Exp $";
#endif

#include <sys/types.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <stdio.h>
#include <pwd.h>
#include "lncmd.h"
#include <X11/Xlib.h>
#include "X11/XWDFile.h"

int debug = 0;

enum device {LN01, LN03, LA100, PS};
enum orientation {PORTRAIT, LANDSCAPE};

#define W_MAX 2400
#define H_MAX 3150
#define W_MARGIN 75
#define H_MARGIN 37
#define W_PAGE 2550
#define H_PAGE 3225

#ifdef NOINLINE
#define min(x,y) (((x)<(y))?(x):(y))
#endif NOINLINE

#define F_PORTRAIT 1
#define F_LANDSCAPE 2
#define F_DUMP 4
#define F_NOSIXOPT 8
#define F_APPEND 16
#define F_NOFF 32
#define F_REPORT 64
#define F_COMPACT 128

char *infilename = "stdin";

main(argc, argv)
char **argv;
{
    unsigned long swaptest = 1;
    XWDFileHeader win;
    register unsigned char (*sixmap)[];
    register int i;
    register int iw;
    register int ih;
    register int sixel_count;
    char *w_name;
    char *filename;
    char *output_filename;
    int scale, width, height, flags, split;
    int left, top;
    int top_margin, left_margin;
    int hpad;
    char *header, *trailer;
    enum orientation orientation;
    enum device device;
    
    parse_args (argc, argv, &scale, &width, &height, &left, &top, &device, 
		&flags, &split, &header, &trailer);

    /* read in window header */
    read(0, &win, sizeof win);
    if (*(char *) &swaptest)
	_swaplong((char *) &win, sizeof(win));

    if (win.file_version != XWD_FILE_VERSION) {
	fprintf(stderr,"xpr: file format version missmatch.\n");
	exit(1);
    }
    if (win.header_size < sizeof(win)) {
	fprintf(stderr,"xpr: header size is too small.\n");
	exit(1);
    }

    if (win.pixmap_depth != 1 && win.pixmap_format != XYPixmap) {
        fprintf(stderr,"xpr: image is not in XY format");
	exit(1);
    }

    if (win.byte_order != win.bitmap_bit_order)
        fprintf(stderr,"xpr: image will be incorrect, byte swapping required but not performed.\n");

    w_name = (char *)malloc(win.header_size - sizeof win);
    read(0, w_name, win.header_size - sizeof win);
    
    if(win.ncolors)
	read(0,
	     malloc(win.ncolors * sizeof(XColor)),
	     win.ncolors * sizeof(XColor));

    /* calculate orientation and scale */
    setup_layout(device, win.pixmap_width, win.pixmap_height, flags, width, 
		 height, header, trailer, &scale, &orientation);

    if (device == PS) {
	iw = win.pixmap_width;
	ih = win.pixmap_height;
    } else {
	/* calculate w and h cell count */
	iw = win.pixmap_width;
	ih = (win.pixmap_height + 5) / 6;
	hpad = (ih * 6) - win.pixmap_height;

	/* build pixcells from input file */
	sixel_count = iw * ih;
	sixmap = (unsigned char (*)[])malloc(sixel_count);
	build_sixmap(iw, ih, sixmap, hpad, &win);
    }

    /* output commands and sixel graphics */
    if (device == LN03) {
/*	ln03_grind_fonts(sixmap, iw, ih, scale, &pixmap); */
	ln03_setup(iw, ih, orientation, scale, left, top,
		   &left_margin, &top_margin, flags, header, trailer);
	ln03_output_sixels(sixmap, iw, ih, (flags & F_NOSIXOPT), split, 
			   scale, top_margin, left_margin);
	ln03_finish();
    } else if (device == LA100) {
	la100_setup(iw, ih, scale);
	la100_output_sixels(sixmap, iw, ih, (flags & F_NOSIXOPT));
	la100_finish();
    } else if (device == PS) {
	ps_setup(iw, ih, orientation, scale, left, top,
		   flags, header, trailer, w_name);
	ps_output_bits(iw, ih, flags, orientation, &win);
	ps_finish();
    } else {
	fprintf(stderr, "xpr: device not supported\n");
    }
    
    /* print some statistics */
    if (flags & F_REPORT) {
	fprintf(stderr, "Name: %s\n", w_name);
	fprintf(stderr, "Width: %d, Height: %d\n", win.pixmap_width, 
		win.pixmap_height);
	fprintf(stderr, "Orientation: %s, Scale: %d\n", 
		(orientation==PORTRAIT) ? "Portrait" : "Landscape", scale);
    }
    if (device != PS && (flags & F_DUMP)) dump_sixmap(sixmap, iw, ih);
}

parse_args(argc, argv, scale, width, height, left, top, device, flags, 
	   split, header, trailer)
register int argc;
register char **argv;
int *scale;
int *width;
int *height;
int *left;
int *top;
enum device *device;
int *flags;
int *split;
char **header;
char **trailer;
{
    register char *output_filename;
    register int f;
    register int len;
    register int pos;
    double atof();
    int atoi();

    output_filename = NULL;
    *device = LN03;	/* default */
    *flags = 0;
    *split = 1;
    *width = -1;
    *height = -1;
    *top = -1;
    *left = -1;
    *header = NULL;
    *trailer = NULL;
    
    argc--;
    argv++;

    while (argc > 0 && argv[0][0] == '-') {
	len = strlen(*argv);
	switch (argv[0][1]) {
	case 'a':		/* -append <filename> */
	    if (!bcmp(*argv, "-append", len)) {
		argc--; argv++;
		output_filename = *argv;
		*flags |= F_APPEND;
	    }
	    break;

	case 'd':		/* -device {ln03 | la100 | ps | lw} | -dump */
	    if (len <= 2) {
		fprintf(stderr, "xpr: ambiguous option: \"%s\"\n", *argv);
		exit(1);
	    }
	    if (!bcmp(*argv, "-device", len)) {
		argc--; argv++;
		len = strlen(*argv);
		if (!bcmp(*argv, "ln03", len)) {
		    *device = LN03;
		} else if (!bcmp(*argv, "la100", len)) {
		    *device = LA100;
		} else if (!bcmp(*argv, "ps", len)) {
		    *device = PS;
		} else if (!bcmp(*argv, "lw", len)) {
		    *device = PS;
		} else {
		    fprintf(stderr, 
			    "xpr: device \"%s\" not supported\n", *argv);
		    exit(1);
		}
	    } else if (!bcmp(*argv, "-dump", len)) {
		*flags |= F_DUMP;
	    }
	    break;

	case 'h':		/* -height <inches> */
	    if (len <= 3) {
		fprintf(stderr, "xpr: ambiguous option: \"%s\"\n", *argv);
		exit(1);
	    }
	    if (!bcmp(*argv, "-height", len)) {
		argc--; argv++;
		*height = (int)(300.0 * atof(*argv));
	    } else if (!bcmp(*argv, "-header", len)) {
		argc--; argv++;
		*header = *argv;
	    }
	    break;

	case 'l':		/* -landscape | -left <inches> */
	    if (!bcmp(*argv, "-landscape", len)) {
		*flags |= F_LANDSCAPE;
	    } else if (!bcmp(*argv, "-left", len)) {
		argc--; argv++;
		*left = (int)(300.0 * atof(*argv));
	    }
	    break;

	case 'n':		/* -nosixopt | -noff */
	    if (len <= 3) {
		fprintf(stderr, "xpr: ambiguous option: \"%s\"\n", *argv);
		exit(1);
	    }
	    if (!bcmp(*argv, "-nosixopt", len)) {
		*flags |= F_NOSIXOPT;
	    } else if (!bcmp(*argv, "-noff", len)) {
		*flags |= F_NOFF;
	    }
	    break;

	case 'o':		/* -output <filename> */
	    if (!bcmp(*argv, "-output", len)) {
		argc--; argv++;
		output_filename = *argv;
	    }
	    break;

	case 'p':		/* -portrait */
	    if (!bcmp(*argv, "-portrait", len)) {
		*flags |= F_PORTRAIT;
	    }
	    break;

	case 'c':		/* -compact */
	    if (!bcmp(*argv, "-compact", len)) {
		*flags |= F_COMPACT;
	    }
	    break;

	case 'r':		/* -report */
	    if (!bcmp(*argv, "-report", len)) {
		*flags |= F_REPORT;
	    }
	    break;

	case 's':		/* -scale <scale> | -split <n-pages> */
	    if (!bcmp(*argv, "-scale", len)) {
		argc--; argv++;
		*scale = atoi(*argv);
	    } else if (!bcmp(*argv, "-split", len)) {
		argc--; argv++;
		*split = atoi(*argv);
	    }
	    break;

	case 't':		/* -top <inches> */
	    if (len <= 2) {
		fprintf(stderr, "xpr: ambigous option: \"%s\"\n", *argv);
		exit(1);
	    }
	    if (!bcmp(*argv, "-top", len)) {
		argc--; argv++;
		*top = (int)(300.0 * atof(*argv));
	    } else if (!bcmp(*argv, "-trailer", len)) {
		argc--; argv++;
		*trailer = *argv;
	    }
	    break;

	case 'w':		/* -width <inches> */
	    if (!bcmp(*argv, "-width", len)) {
		argc--; argv++;
		*width = (int)(300.0 * atof(*argv));
	    }
	    break;

	}
	argc--; argv++;
    }

    if (argc > 0) {
	f = open(*argv, O_RDONLY, 0);
	if (f < 0) {
	    fprintf(stderr, "xpr: error opening \"%s\" for input\n", *argv);
	    perror("");
	    exit(1);
	}
	dup2(f, 0);
	close(f);
	infilename = *argv;
/*	if (output_filename == NULL) {
	    output_filename = (char *)malloc(strlen(*argv)+10);
	    build_output_filename(*argv, *device, output_filename);
	} */
    }

    if (output_filename != NULL) {
	if (!(*flags & F_APPEND)) {
	    f = open(output_filename, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	} else {
	    f = open(output_filename, O_WRONLY, 0);
	}
	if (f < 0) {
	    fprintf(stderr, "xpr: error opening \"%s\" for output\n", 
		    output_filename);
	    perror("xpr");
	    exit(1);
	}
	if (*flags & F_APPEND) {
	    pos = lseek(f, 0, 2);          /* get eof position */
	    if (*flags & F_NOFF) pos -= 3; /* set position before trailing */
					   /*     formfeed and reset */
	    lseek(f, pos, 0);              /* set pointer */
	}
	dup2(f, 1);
	close(f);
    }
}

setup_layout(device, win_width, win_height, flags, width, height, 
	     header, trailer, scale, orientation)
enum device device;
int win_width;
int win_height;
int flags;
int width;
int height;
char *header;
char *trailer;
int *scale;
enum orientation *orientation;
{
    register int w_scale;
    register int h_scale;
    register int iscale = *scale;
    register int w_max;
    register int h_max;

    if (header != NULL) win_height += 75;
    if (trailer != NULL) win_height += 75;

    /* check maximum width and height; set orientation and scale*/
    if (device == LN03 || device == PS) {
	if ((win_width < win_height || (flags & F_PORTRAIT)) && 
	    !(flags & F_LANDSCAPE)) {
	    *orientation = PORTRAIT;
	    w_max = (width > 0)? width : W_MAX;
	    h_max = (height > 0)? height : H_MAX;
	    w_scale = w_max / win_width;
	    h_scale = h_max / win_height;
	    *scale = min(w_scale, h_scale);
	} else {
	    *orientation = LANDSCAPE;
	    w_max = (width > 0)? width : H_MAX;
	    h_max = (height > 0)? height : W_MAX;
	    w_scale = w_max / win_width;
	    h_scale = h_max / win_height;
	    *scale = min(w_scale, h_scale);
	}
    } else {			/* device == LA100 */
	*orientation = PORTRAIT;
	*scale = W_MAX / win_width;
    }
    if (*scale == 0) *scale = 1;
    if (*scale > 6) *scale = 6;
    if (iscale > 0 && iscale < *scale) *scale = iscale;
}

dump_sixmap(sixmap, iw, ih)
register unsigned char (*sixmap)[];
int iw;
int ih;
{
    register int i, j;
    register unsigned char *c;

    c = (unsigned char *)sixmap;
    fprintf(stderr, "Sixmap:\n");
    for (i = 0; i < ih; i++) {
	for (j = 0; j < iw; j++) {
	    fprintf(stderr, "%02X ", *c++);
	}
	fprintf(stderr, "\n\n");
    }
}

build_sixmap(iw, ih, sixmap, hpad, win)
int ih;
int iw;
unsigned char (*sixmap)[];
int hpad;
XWDFileHeader *win;
{
    int iwb = win->bytes_per_line;
    int iww;
    int rsize, cc;
    int w, maxw;
    struct iovec linevec[6];
    unsigned char line[6][500];
    register unsigned char *c;
    register int i, j, k, m;
    register int sixel;
    static int mask[] = {~1, ~2, ~4, ~8, ~16, ~32, ~64, ~128};

    c = (unsigned char *)sixmap;

    for (i = 0; i <= 5; i++) {
	linevec[i].iov_base = (caddr_t)line[i];
	linevec[i].iov_len = iwb;
    }

    while (--ih >= 0) {
	if (ih > 0 || hpad == 0) {
	    rsize = iwb * 6;
	    while (rsize > 0) {
		cc = readv(0, linevec, 6);
		if (cc == 0) break;
		rsize -= cc;
	    }
	} else {
	    i = 6 - hpad;
	    rsize = iwb * i;
	    while (rsize > 0) {
		cc = readv(0, linevec, i);
		if (cc == 0) break;
		rsize -= cc;
	    }
	    for (; i < 6; i++)
		for (j = 0; j < iwb; j++) line[i][j] = 0xFF;
	}

	if (win->bitmap_bit_order == MSBFirst)
	    for (i = 0; i <= 5; i++)
	        _swapbits((char *)&line[i][0], iwb);

#ifndef NOINLINE
	for (i = 0; i < iw; i++) {
	    sixel =  extzv(line[0], i, 1);
	    sixel |= extzv(line[1], i, 1) << 1;
	    sixel |= extzv(line[2], i, 1) << 2;
	    sixel |= extzv(line[3], i, 1) << 3;
	    sixel |= extzv(line[4], i, 1) << 4;
	    sixel |= extzv(line[5], i, 1) << 5;
	    *c++ = sixel;
	}
#else
	for (i = 0, w = iw; w > 0; i++) {
	    for (j = 0; j <= 7; j++) {
		m = mask[j];
		k = -j;
		sixel =  ((line[0][i] & ~m) << k++);
		sixel |= ((line[1][i] & ~m) << k++);
		sixel |= ((line[2][i] & ~m) << k++);
		sixel |= ((line[3][i] & ~m) << k++);
		sixel |= ((line[4][i] & ~m) << k++);
		sixel |= ((line[5][i] & ~m) << k);
		*c++ = sixel;
		if (--w == 0) break;
	    }
	}
#endif
    }
}

build_output_filename(name, device, oname)
register char *name, *oname;
enum device device;
{
    while (*name && *name != '.') *oname++ = *name++;
    switch (device) {
    case LN03:	bcopy(".ln03", oname, 6); break;
    case LA100:	bcopy(".la100", oname, 7); break;
    }
}

/*
ln03_grind_fonts(sixmap, iw, ih, scale, pixmap)
unsigned char (*sixmap)[];
int iw;
int ih;
int scale;
struct pixmap (**pixmap)[];
{
}
*/

ln03_setup(iw, ih, orientation, scale, left, top, left_margin, top_margin, 
	   flags, header, trailer)
int iw;
int ih;
enum orientation orientation;
int scale;
int left;
int top;
int *left_margin;
int *top_margin;
int flags;
char *header;
char *trailer;
{
    register int i;
    register int lm, tm, xm;
    char fontname[6];
    char buf[256];
    register char *bp = buf;
	
    if (!(flags & F_APPEND)) {
	sprintf(bp, LN_RIS); bp += 2;
	sprintf(bp, LN_SSU, 7); bp += 5;
	sprintf(bp, LN_PUM_SET); bp += sizeof LN_PUM_SET - 1;
    }

    if (orientation == PORTRAIT) {
	lm = (left > 0)? left : (((W_MAX - scale * iw) / 2) + W_MARGIN);
	tm = (top > 0)? top : (((H_MAX - scale * ih * 6) / 2) + H_MARGIN);
	sprintf(bp, LN_PFS, "?20"); bp += 7;
	sprintf(bp, LN_DECOPM_SET); bp += sizeof LN_DECOPM_SET - 1;
	sprintf(bp, LN_DECSLRM, lm, W_PAGE - lm); bp += strlen(bp);
    } else {
	lm = (left > 0)? left : (((H_MAX - scale * iw) / 2) + H_MARGIN);
	tm = (top > 0)? top : (((W_MAX - scale * ih * 6) / 2) + W_MARGIN);
	sprintf(bp, LN_PFS, "?21"); bp += 7;
	sprintf(bp, LN_DECOPM_SET); bp += sizeof LN_DECOPM_SET - 1;
	sprintf(bp, LN_DECSLRM, lm, H_PAGE - lm); bp += strlen(bp);
    }

    if (header != NULL) {
	sprintf(bp, LN_VPA, tm - 100); bp += strlen(bp);
	i = strlen(header);
	xm = (((scale * iw) - (i * 30)) / 2) + lm;
	sprintf(bp, LN_HPA, xm); bp += strlen(bp);
	sprintf(bp, LN_SGR, 3); bp += strlen(bp);
	bcopy(header, bp, i);
	bp += i;
    }
    if (trailer != NULL) {
	sprintf(bp, LN_VPA, tm + (scale * ih * 6) + 75); bp += strlen(bp);
	i = strlen(trailer);
	xm = (((scale * iw) - (i * 30)) / 2) + lm;
	sprintf(bp, LN_HPA, xm); bp += strlen(bp);
	sprintf(bp, LN_SGR, 3); bp += strlen(bp);
	bcopy(trailer, bp, i);
	bp += i;
    }

    sprintf(bp, LN_HPA, lm); bp += strlen(bp);
    sprintf(bp, LN_VPA, tm); bp += strlen(bp);
    sprintf(bp, LN_SIXEL_GRAPHICS, 9, 0, scale); bp += strlen(bp);
    sprintf(bp, "\"1;1"); bp += 4; /* Pixel aspect ratio */
    write(1, buf, bp-buf);
    *top_margin = tm;
    *left_margin = lm;
}

#define LN03_RESET "\033c"

ln03_finish()
{
    write(1, LN03_RESET, sizeof LN03_RESET - 1);
}

la100_setup(iw, ih, scale)
{
    unsigned char buf[256];
    register unsigned char *bp;
    int lm, tm;

    bp = buf;
    lm = ((80 - (int)((double)iw / 6.6)) / 2) - 1;
    if (lm < 1) lm = 1;
    tm = ((66 - (int)((double)ih / 2)) / 2) - 1;
    if (tm < 1) tm = 1;
    sprintf(bp, "\033[%d;%ds", lm, 81-lm); bp += strlen(bp);
    sprintf(bp, "\033[?7l"); bp += 5;
    sprintf(bp, "\033[%dd", tm); bp += strlen(bp);
    sprintf(bp, "\033[%d`", lm); bp += strlen(bp);
    sprintf(bp, "\033P0q"); bp += 4;
    write(1, buf, bp-buf);
}

#define LA100_RESET "\033[1;80s\033[?7h"

la100_finish()
{
    write(1, LA100_RESET, sizeof LA100_RESET - 1);
}

#define COMMENTVERSION "PS-Adobe-1.0"

#ifdef XPROLOG
/* for debugging, get the prolog from a file */
dump_prolog(flags) {
    char *fname=(flags & F_COMPACT) ? "prolog.compact" : "prolog";
    FILE *fi = fopen(fname,"r");
    char buf[1024];

    if (fi==NULL) {
	perror(fname);
	exit(1);
    }
    while (fgets(buf,1024,fi)) fputs(buf,stdout);
    fclose(fi);
}

#else XPROLOG
/* postscript "programs" to unpack and print the bitmaps being sent */

char *ps_prolog_compact[] = {
    "%%Pages: 1",
    "%%EndProlog",
    "%%Page: 1 1",
    "",
    "/bitgen",
    "	{",
    "		/nextpos 0 def",
    "		currentfile bufspace readhexstring pop % get a chunk of input",
    "		% interpret each byte of the input",
    "		{",
    "			flag { % if the previous byte was FF",
    "				/len exch def % this byte is a count",
    "				result",
    "				nextpos",
    "				FFstring 0 len getinterval % grap a chunk of FF's",
    "					putinterval % and stuff them into the result",
    "				/nextpos nextpos len add def",
    "				/flag false def",
    "			}{ % otherwise",
    "				dup 255 eq { % if this byte is FF",
    "					/flag true def % just set the flag",
    "					pop % and toss the FF",
    "				}{ % otherwise",
    "					% move this byte to the result",
    "					result nextpos",
    "						3 -1 roll % roll the current byte back to the top",
    "						put",
    "					/nextpos nextpos 1 add def",
    "				} ifelse",
    "			} ifelse",
    "		} forall",
    "		% trim unused space from end of result",
    "		result 0 nextpos getinterval",
    "	} def",
    "",
    "",
    "/bitdump % stk: width, height, iscale",
    "	% dump a bit image with lower left corner at current origin,",
    "	% scaling by iscale (iscale=1 means 1/300 inch per pixel)",
    "	{",
    "		% read arguments",
    "		/iscale exch def",
    "		/height exch def",
    "		/width exch def",
    "",
    "		% scale appropriately",
    "		width iscale mul height iscale mul scale",
    "",
    "		% data structures:",
    "",
    "		% allocate space for one line of input",
    "		/bufspace 36 string def",
    "",
    "		% string of FF's",
    "		/FFstring 256 string def",
    "		% for all i FFstring[i]=255",
    "		0 1 255 { FFstring exch 255 put } for",
    "",
    "		% 'escape' flag",
    "		/flag false def",
    "",
    "		% space for a chunk of generated bits",
    "		/result 1000 string def",
    "",
    "		% read and dump the image",
    "		width height 1 [width 0 0 height neg 0 height]",
    "			{ bitgen }",
    "			image",
    "	} def",
    0
};

char *ps_prolog[] = {
    "%%Pages: 1",
    "%%EndProlog",
    "%%Page: 1 1",
    "",
    "/bitdump % stk: width, height, iscale",
    "% dump a bit image with lower left corner at current origin,",
    "% scaling by iscale (iscale=1 means 1/300 inch per pixel)",
    "{",
    "	% read arguments",
    "	/iscale exch def",
    "	/height exch def",
    "	/width exch def",
    "",
    "	% scale appropriately",
    "	width iscale mul height iscale mul scale",
    "",
    "	% allocate space for one scanline of input",
    "	/picstr % picstr holds one scan line",
    "		width 7 add 8 idiv % width of image in bytes = ceiling(width/8)",
    "		string",
    "		def",
    "",
    "	% read and dump the image",
    "	width height 1 [width 0 0 height neg 0 height]",
    "	{ currentfile picstr readhexstring pop }",
    "	image",
    "} def",
    0
};

dump_prolog(flags) {
    char **p = (flags & F_COMPACT) ? ps_prolog_compact : ps_prolog;
    while (*p) printf("%s\n",*p++);
}
#endif XPROLOG

#define PAPER_WIDTH 85*30 /* 8.5 inches */
#define PAPER_LENGTH 11*300 /* 11 inches */

static int
points(n)
{
    /* scale n from pixels (1/300 inch) to points (1/72 inch) */
    n *= 72;
    return n/300;
}

static char *
escape(s)
char *s;
{
    /* make a version of s in which control characters are deleted and
     * special characters are escaped.
     */
    static char buf[200];
    char *p = buf;

    for (;*s;s++) {
	if (*s < ' ' || *s > 0176) continue;
	if (*s==')' || *s=='(' || *s == '\\') {
	    sprintf(p,"\\%03o",*s);
	    p += 4;
	}
	else *p++ = *s;
    }
    *p = 0;
    return buf;
}

/* ARGSUSED */
ps_setup(iw, ih, orientation, scale, left, top, 
	   flags, header, trailer, name)
int iw;
int ih;
enum orientation orientation;
int scale;
int left;
int top;
int flags;
char *header;
char *trailer;
char *name;
{
    char    hostname[256];
    struct passwd  *pswd;
    long    clock;
    int lm, bm; /* left (bottom) margin (paper in portrait orientation) */

    printf ("%%!%s\n", COMMENTVERSION);
    pswd = getpwuid (getuid ());
    (void) gethostname (hostname, sizeof hostname);
    printf ("%%%%Creator: %s:%s (%s)\n", hostname,
	    pswd->pw_name, pswd->pw_gecos);
    printf ("%%%%Title: %s (%s)\n", infilename,name);
    printf ("%%%%CreationDate: %s",
		(time (&clock), ctime (&clock)));
    printf ("%%%%EndComments\n");

    dump_prolog(flags);

    if (orientation==PORTRAIT) {
	lm = (left > 0)? left : ((PAPER_WIDTH - scale * iw) / 2);
	bm = (top > 0)? (PAPER_LENGTH - top - scale * ih)
		: ((PAPER_LENGTH - scale * ih) / 2);
	if (header || trailer) {
	    printf("gsave\n");
	    printf("/Times-Roman findfont 15 scalefont setfont\n");
	    /* origin at bottom left corner of image */
	    printf("%d %d translate\n",points(lm),points(bm));
	    if (header) {
		char *label = escape(header);
		printf("%d (%s) stringwidth pop sub 2 div %d moveto\n",
		    points(iw*scale), label, points(ih*scale) + 10);
		printf("(%s) show\n",label);
	    }
	    if (trailer) {
		char *label = escape(trailer);
		printf("%d (%s) stringwidth pop sub 2 div -20 moveto\n",
		    points(iw*scale), label);
		printf("(%s) show\n",label);
	    }
	    printf("grestore\n");
	}
	/* set resolution to device units (300/inch) */
	printf("72 300 div dup scale\n");
	/* move to lower left corner of image */
	printf("%d %d translate\n",lm,bm);
	/* dump the bitmap */
	printf("%d %d %d bitdump\n",iw,ih,scale);
    } else { /* orientation == LANDSCAPE */
	/* calculate margins */
	lm = (top > 0)? (PAPER_WIDTH - top - scale * ih)
		: ((PAPER_WIDTH - scale * ih) / 2);
	bm = (left > 0)? (PAPER_LENGTH - left - scale * iw)
		: ((PAPER_LENGTH - scale * iw) / 2);

	if (header || trailer) {
	    printf("gsave\n");
	    printf("/Times-Roman findfont 15 scalefont setfont\n");
	    /* origin at top left corner of image */
	    printf("%d %d translate\n",points(lm),points(bm + scale * iw));
	    /* rotate to print the titles */
	    printf("-90 rotate\n");
	    if (header) {
		char *label = escape(header);
		printf("%d (%s) stringwidth pop sub 2 div %d moveto\n",
		    points(iw*scale), label, points(ih*scale) + 10);
		printf("(%s) show\n",label);
	    }
	    if (trailer) {
		char *label = escape(trailer);
		printf("%d (%s) stringwidth pop sub 2 div -20 moveto\n",
		    points(iw*scale), label);
		printf("(%s) show\n",label);
	    }
	    printf("grestore\n");
	}
	/* set resolution to device units (300/inch) */
	printf("72 300 div dup scale\n");
	/* move to lower left corner of image */
	printf("%d %d translate\n",lm,bm);
	/* dump the bitmap */
	printf("%d %d %d bitdump\n",ih,iw,scale);
    }
}

char *ps_epilog[] = {
	"",
	"showpage",
	"%%Trailer",
	0
};

ps_finish()
{
	char **p = ps_epilog;

	while (*p) printf("%s\n",*p++);
}

ln03_alter_background(sixmap, iw, ih)
unsigned char (*sixmap)[];
int iw;
int ih;
{
    register int size;
    register unsigned char *c, *stopc;
    register unsigned char *startc;
    register int n;

    c = (unsigned char *)sixmap;
    stopc = c + (iw * ih);
    n = 0;
    while (c < stopc) {
	switch (*c) {
	case 0x08: case 0x11: case 0x04: case 0x22:
	case 0x20: case 0x21: case 0x24: case 0x00:
	    if (n == 0) startc = c;
	    n++;
	    break;

	default:
	    if (n >= 2) {
		while (n-- > 0) *startc++ = 0x00;
	    } else {
		n = 0;
	    }
	    break;
	}
	c++;
    }
}

ln03_output_sixels(sixmap, iw, ih, nosixopt, split, scale, top_margin, 
		   left_margin)
unsigned char (*sixmap)[];
int iw;
int ih;
int nosixopt;
int split;
int top_margin;
int left_margin;
{
    unsigned char *buf;
    register unsigned char *bp;
    int i;
    int j;
    register int k;
    register unsigned char *c;
    register int lastc;
    register int count;
    char snum[6];
    register char *snp;

    bp = (unsigned char *)malloc(iw*ih+512);
    buf = bp;
    count = 0;
    lastc = -1;
    c = (unsigned char *)sixmap;
    split = ih / split;		/* number of lines per page */

    iw--;			/* optimization */
    for (i = 0; i < ih; i++) {
	for (j = 0; j <= iw; j++) {
	    if (!nosixopt) {
		if (*c == lastc && j < iw) {
		    count++;
		    c++;
		    continue;
		}
		if (count >= 3) {
		    bp--;
		    count++;
		    *bp++ = '!';
		    snp = snum;
		    while (count > 0) {
			k = count / 10;
			*snp++ = count - (k * 10) + '0';
			count = k;
		    }
		    while (--snp >= snum) *bp++ = *snp;
		    *bp++ = (~lastc & 0x3F) + 0x3F;
		} else if (count > 0) {
		    lastc = (~lastc & 0x3F) + 0x3F;
		    do {
			*bp++ = lastc;
		    } while (--count > 0);
		}
	    }
	    lastc = *c++;
	    *bp++ = (~lastc & 0x3F) + 0x3F;
	}
	*bp++ = '-';		/* New line */
	lastc = -1;
	if ((i % split) == 0 && i != 0) {
	    sprintf(bp, LN_ST); bp += sizeof LN_ST - 1;
	    *bp++ = '\f';
	    sprintf(bp, LN_VPA, top_margin + (i * 6 * scale)); bp += strlen(bp);
	    sprintf(bp, LN_HPA, left_margin); bp += strlen(bp);
	    sprintf(bp, LN_SIXEL_GRAPHICS, 9, 0, scale); bp += strlen(bp);
	    sprintf(bp, "\"1;1"); bp += 4;
	}
    }

    sprintf(bp, LN_ST); bp += sizeof LN_ST - 1;
    *bp++ = '\f';
    write(1, buf, bp-buf);
}

la100_output_sixels(sixmap, iw, ih)
unsigned char (*sixmap)[];
int iw;
int ih;
{
    unsigned char *buf;
    register unsigned char *bp;
    int i;
    register int j, k;
    register unsigned char *c;
    register int lastc;
    register int count;
    char snum[6];

    bp = (unsigned char *)malloc(iw*ih+512);
    buf = bp;
    count = 0;
    lastc = -1;
    c = (unsigned char *)sixmap;

    for (i = 0; i < ih; i++) {
	for (j = 0; j < iw; j++) {
	    if (*c == lastc && (j+1) < iw) {
		count++;
		c++;
		continue;
	    }
	    if (count >= 2) {
		bp -= 2;
		count = 2 * (count + 1);
		*bp++ = '!';
		k = 0;
		while (count > 0) {
		    snum[k++] = (count % 10) + '0';
		    count /= 10;
		}
		while (--k >= 0) *bp++ = snum[k];
		*bp++ = (~lastc & 0x3F) + 0x3F;
		count = 0;
	    } else if (count > 0) {
		lastc = (~lastc & 0x3F) + 0x3F;
		do {
		    *bp++ = lastc;
		    *bp++ = lastc;
		} while (--count > 0);
	    }
	    lastc = (~*c & 0x3F) + 0x3F;
	    *bp++ = lastc;
	    *bp++ = lastc;
	    lastc = *c++;
	}
	*bp++ = '-';		/* New line */
	lastc = -1;
    }

    sprintf(bp, LN_ST); bp += sizeof LN_ST - 1;
    *bp++ = '\f';
    write(1, buf, bp-buf);
}

#define LINELEN 72 /* number of CHARS (bytes*2) per line of bitmap output */
char *obuf; /* buffer to contain entire rotated bit map */

ps_output_bits(iw, ih, flags, orientation, win)
int iw;
int ih;
int flags;
XWDFileHeader *win;
enum orientation orientation;
{
    int iwb = win->bytes_per_line;
    register int i;
    int n,bytes;
    unsigned char *buffer;
    register int ocount=0;
    extern char hex1[],hex2[];
    static char hex[] = "0123456789abcdef";

    buffer = (unsigned char *)malloc((unsigned)(iwb + 3));
    if (orientation == LANDSCAPE) {
	/* read in and rotate the entire image */
	/* The Postscript language has a rotate operator, but using it
	 * seem to make printing (at least on the Apple Laserwriter
	 * take about 10 times as long (40 minutes for a 1024x864 full-screen
	 * dump)!  Therefore, we rotate the image here.
	 */
	int ocol = ih;
	int owidth = (ih+31)/32; /* width of rotated image, in bytes */
	int oheight = (iw+31)/32; /* height of rotated image, in scanlines */
	register char *p, *q;
	owidth *= 4;
	oheight *= 32;

	/* Allocate buffer for the entire rotated image (output).
	 * Owidth and Oheight are rounded up to a multiple of 32 bits,
	 * to avoid special cases at the boundaries
	 */
	obuf = (char *)malloc((unsigned)(owidth*oheight));
	if (obuf==0) {
	    fprintf(stderr,"xpr: cannot allocate %d bytes\n",owidth*oheight);
	    exit(1);
	}
	bzero(obuf,owidth*oheight);

	for (i=0;i<ih;i++) {
	    n = read(0,(char *)buffer,iwb);
	    if (n<0) {
		perror("read");
		exit(1);
	    }
	    if (n==0) {
		fprintf(stderr,"xpr: premature end of file\n");
		exit(1);
	    }
	    if (win->bitmap_bit_order == MSBFirst)
		_swapbits((char *)buffer, iwb);
	    ps_bitrot(buffer,iw,--ocol,owidth);
	}
	q = &obuf[iw*owidth];
	bytes = (ih+7)/8;
	for (p=obuf;p<q;p+=owidth)
	    ocount = ps_putbuf((unsigned char *)p,bytes,ocount,flags&F_COMPACT);
    }
    else {
	for (i=0;i<ih;i++) {
	    n = read(0,(char *)buffer,iwb);
	    if (n<0) {
		perror("read");
		exit(1);
	    }
	    if (n==0) {
		fprintf(stderr,"xpr: premature end of file\n");
		exit(1);
	    }
	    if (win->bitmap_bit_order == MSBFirst)
		_swapbits((char *)buffer, iwb);
	    ocount = ps_putbuf(buffer,iwb,ocount,flags&F_COMPACT);
	}
    }
    if (flags & F_COMPACT) {
	if (ocount) {
	    /* pad to an integral number of lines */
	    while (ocount++ < LINELEN)
		/* for debugging, pad with a "random" value */
		putchar(hex[ocount&15]);
	    putchar('\n');
	}
    }
}

/* Dump some bytes in hex, with bits in each byte reversed
 * Ocount is number of chacters that have been written to the current
 * output line.  It's new value is returned as the result of the function.
 * Ocount is ignored (and the return value is meaningless) if compact==0.
 */
int
ps_putbuf(s, n, ocount, compact)
register unsigned char *s;	/* buffer to dump */
register int n;			/* number of BITS to dump */
register int ocount;		/* position on output line for next char */
int compact;			/* if non-zero, do compaction (see below) */
{
    register int ffcount = 0;
    extern char hex1[],hex2[];
    static char hex[] = "0123456789abcdef";
#define PUT(c) { putchar(c); if (++ocount>=LINELEN) \
	{ putchar('\n'); ocount=0; }}

    if (compact) {
	/* The following loop puts out the bits of the image in hex,
	 * compressing runs of white space (represented by one bits)
	 * according the the following simple algorithm:  A run of n
	 * 'ff' bytes (i.e., bytes with value 255--all ones), where
	 * 1<=n<=255, is represented by a single 'ff' byte followed by a
	 * byte containing n.
	 * On a typical dump of a full screen pretty much covered by
	 * black-on-white text windows, this compression decreased the
	 * size of the file from 223 Kbytes to 63 Kbytes.
	 * Of course, another factor of two could be saved by sending
	 * the bytes 'as is' rather than in hex, using some sort of
	 * escape convention to avoid problems with control characters.
	 * Another popular encoding is to pack three bytes into 4 'sixels'
	 * as in the LN03, etc, but I'm too lazy to write the necessary
	 * PostScript code to unpack fancier representations.
	 */
	while (n--) {
	    if (*s == 0xff) {
		if (++ffcount == 255) {
		    PUT('f'); PUT('f');
		    PUT('f'); PUT('f');
		    ffcount = 0;
		}
	    }
	    else {
		if (ffcount) {
		    PUT('f'); PUT('f');
		    PUT(hex[ffcount >> 4]);
		    PUT(hex[ffcount & 0xf]);
		    ffcount = 0;
		}
		PUT(hex1[*s]);
		PUT(hex2[*s]);
	    }
	    s++;
	}
	if (ffcount) {
	    PUT('f'); PUT('f');
		PUT(hex[ffcount >> 4]);
	    PUT(hex[ffcount & 0xf]);
	    ffcount = 0;
	}
    }
    else { /* no compaction: just dump the image in hex (bits reversed) */
	while (n--) {
	    putchar(hex1[*s]);
	    putchar(hex2[*s++]);
	}
	putchar('\n');
    }
    return ocount;
}

ps_bitrot(s,n,col,owidth)
unsigned char *s;
register int n;
int col;
register int owidth;
/* s points to a chunk of memory and n is its width in bits.
 * The algorithm is, roughly,
 *    for (i=0;i<n;i++) {
 *        OR the ith bit of s into the ith row of the
 *        (col)th column of obuf
 *    }
 * Assume VAX bit and byte ordering for s:
 *	The ith bit of s is s[j]&(1<<k) where i=8*j+k.
 *	It can also be retrieved as t[j]&(1<<k), where t=(int*)s and i=32*j+k.
 * Also assume VAX bit and byte ordering for each row of obuf.
 * Ps_putbuf() takes care of converting to Motorola 68000 byte and bit
 * ordering.  The following code is very carefully tuned to yield a very
 * tight loop on the VAX, since it easily dominates the entire running
 * time of this program.  In particular, iwordp is declared last, since
 * there aren't enough registers, and iwordp is referenced only once
 * every 32 times through the loop.
 */
{
    register int mask = 1<<(col%32);
    register int iword; /* current input word (*iwordp) */
    register int b = 0; /* number of bits in iword left to examine */
    register char *opos = obuf + (col/32)*4;
	/* pointer to word of obuf to receive next output bit */
    register int *iwordp = (int *) s; /* pointer to next word of s */

    while (--n>=0) {
	if (--b < 0) {
	    iword = *iwordp++;
	    b = 31;
	}
	if (iword & 1) {
	    *(int *)opos |= mask;
	}
	opos += owidth;
	iword >>= 1;
    }
}

/* mapping tables to map a byte in to the hex representation of its
 * bit-reversal
 */
char hex1[]="084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f\
084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f\
084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f\
084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f084c2a6e195d3b7f";

char hex2[]="000000000000000088888888888888884444444444444444cccccccccccccccc\
2222222222222222aaaaaaaaaaaaaaaa6666666666666666eeeeeeeeeeeeeeee\
111111111111111199999999999999995555555555555555dddddddddddddddd\
3333333333333333bbbbbbbbbbbbbbbb7777777777777777ffffffffffffffff";

