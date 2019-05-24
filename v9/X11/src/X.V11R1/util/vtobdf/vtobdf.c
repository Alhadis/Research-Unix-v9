#ifndef lint
static char rcsid[] =
    "@(#) $Header: vtobdf.c,v 1.1 87/09/11 08:16:28 toddb Exp $ (LBL)";
#endif

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <sys/file.h>
/*
#include <sys/param.h>
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <vfont.h>

#define SPACE			32	/* space index */
#define DELETE			127	/* delete index */
#define LAST_CHAR		127	/* delete */
#define FIRST_CHAR		0	/* null */
#define NUM_CHAR		128	/* total number of characters */

static void
punt(str)
caddr_t	str;
{
	fprintf(stderr, "%s\n", str);
	perror("vtobdf");
	exit(1);
}

static void
vtobdf(vfile, bdffile, bdfname)
	FILE *vfile, *bdffile;
	char *bdfname;
{
	int		i;
	int j;
	struct stat		stb;
	struct header		header;
	struct dispatch		dispatch[NUM_DISPATCH];
	int			bitmapWidth;
	int			maxUp, maxDown, maxWidth, maxHeight;
	int			fixed;
	int fontsize;
	register char *cp;
	char buf[2048];
	int num_chars;

#define FONTBASE	(sizeof(header) + sizeof(dispatch))

	/* read vfont header */
	if (read(fileno(vfile), (caddr_t) &header, sizeof(header)) != sizeof(header))
		punt("Error reading vfont header.");

	/* check magic number */
	if (header.magic != VFONT_MAGIC)
		punt("Not VFONT_MAGIC.");

	/* read vfont dispatch */
	if (read(fileno(vfile), (caddr_t) dispatch, sizeof(dispatch)) != 
		sizeof(dispatch))
		punt("Error reading vfont dispatch table.");

	/* stat the file descriptor to check file size */
	if (fstat(fileno(vfile), &stb) < 0) {
		perror(vfile);
		exit(1);
	}

	/* check file size */
	if (stb.st_size != FONTBASE + header.size)
		punt("Invalid file size.");

	/* Figure out font size */
	fontsize = 6;	/* default */
	cp = rindex(bdfname, '-');
	if (cp) {
		cp++;
		if (sscanf(cp, "%d", &i) == 1 && i > 0)
			fontsize = i;
	}

        /* initialize */
        num_chars = maxWidth = maxUp = maxDown = 0;
        /* determine maximum width, up and down for font */
	for (i = 0, bitmapWidth = 0, fixed = 0; i < NUM_CHAR; i++) {
		register struct dispatch	*d;
		register int			width;

		d = &dispatch[i + FIRST_CHAR];

		if (d->nbytes)
			num_chars++;

		width = d->left + d->right;
		if (i == 0)
			fixed = width;
		else if (width != fixed)
			fixed = 0;
		if (width > maxWidth)
			maxWidth = width;
		if (d->up > maxUp)
			maxUp = d->up;
		if (d->down > maxDown)
			maxDown = d->down;
		bitmapWidth += width;
	}
	maxHeight = maxUp + maxDown;

	/* Start spewing stuff out */
	fprintf(bdffile, "STARTFONT 2.1\n");
	fprintf(bdffile, "FONT %s\n", bdfname);
	fprintf(bdffile, "SIZE %d 78 78\n", fontsize);
	fprintf(bdffile, "FONTBOUNDINGBOX %d %d %d %d\n",
	    header.maxx, header.maxy, 0, -maxDown);
	fprintf(bdffile, "STARTPROPERTIES 3\n");
	fprintf(bdffile, "FONT_ASCENT %d\n", maxUp);
	fprintf(bdffile, "FONT_DESCENT %d\n", maxDown);
	fprintf(bdffile, "DEFAULT_CHAR %d\n", 040);
	fprintf(bdffile, "ENDPROPERTIES\n");
	fprintf(bdffile, "CHARS %d\n", num_chars);

	for (i = 0; i < NUM_CHAR; i++) {
		struct dispatch		*d;
		int			width, height;

		/* entry in dispatch array */
		d = &dispatch[i + FIRST_CHAR];

		/* check to see if this character is defined */
		if (d->nbytes == 0)
			continue;

		if (isprint(i) && !isspace(i))
			fprintf(bdffile, "STARTCHAR %c\n", i);
		else
			fprintf(bdffile, "STARTCHAR C%03o\n", i);
		fprintf(bdffile, "ENCODING %d\n", i);

		/* width in pixels */
		width = d->left + d->right;

		fprintf(bdffile, "SWIDTH %d %d\n", fontsize * width, 0);
		fprintf(bdffile, "DWIDTH %d %d\n", width, 0);

		/* height in pixels */
		height = d->up + d->down;

		fprintf(bdffile, "BBX %d %d %d %d\n",
		    width, height, d->left, -d->down);

		fprintf(bdffile, "BITMAP\n");

		if (d->addr > sizeof(buf)) {
			fprintf(stderr, "d->addr is %d, bufsize is %d",
			    d->addr, sizeof(buf));
			punt("foo");
		}
		if (lseek(fileno(vfile), FONTBASE + d->addr, 0) < 0)
			punt("Can't seek in vfont file.");
		if (read(fileno(vfile), buf, d->nbytes) != d->nbytes)
			punt("Error reading vfont data.");
		cp = buf;
		for (j = d->nbytes; j > 0; j--) {
			int k;
			k = (unsigned char)*cp;
			cp++;
			fprintf(bdffile, "%02x\n", k);
		}

		fprintf(bdffile, "ENDCHAR\n");
	}

	fprintf(bdffile, "ENDFONT\n");

}

main(argc, argv)
	int argc;
	char **argv;
{
	char *vname, *bdfname;
	FILE *vfile, *bdffile;

	if (argc != 3) {
		fprintf(stderr, "vtobdf: vname bdfname\n");
		exit(1);
	}

	vname = argv[1];
	bdfname = argv[2];

	if ((vfile = fopen(vname, "r")) == NULL) {
		perror(vname);
		exit(1);
	}

	if ((bdffile = fopen(bdfname, "w")) == NULL) {
		perror(bdfname);
		exit(1);
	}

	vtobdf(vfile, bdffile, bdfname);

	fclose(vfile);
	fclose(bdffile);

	exit(0);
}
