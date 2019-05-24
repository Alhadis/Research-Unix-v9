/* 
 * $Locker:  $ 
 */ 
static char	*rcsid = "$Header: showsnf.c,v 1.3 87/07/24 14:54:26 toddb Exp $";
#include <stdio.h>
#include <sys/types.h> 
#include <sys/file.h> 
#include <sys/stat.h> 
#include <errno.h> 
/* #include <malloc.h> */	extern char *malloc(), *realloc();

#include "misc.h"
#include "Xmd.h"
#include "X.h"
#include "Xproto.h"
#include "fontstruct.h"
#include "font.h"

#include "fc.h"	/* used by converters only */
char *program;

int glyphPad = DEFAULTGLPAD;
int bitorder = DEFAULTBITORDER;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	verbose = 0,
		fontcnt = 0;

	program = *argv;
	for (argc--, argv++; argc; argc--, argv++) {
		if (argv[0][0] == '-')
			switch(argv[0][1]) {
			case 'v':	verbose++; break;
			case 'm':	bitorder = MSBFirst; break;
			case 'l':	bitorder = LSBFirst; break;
			default:	usage(); break;
			}
		else {
			showfont(argv[0], verbose);
			fontcnt++;
		}
	}
	if (!fontcnt)
		usage();
	exit(0);
}

usage()
{
	fprintf(stderr, "Usage: %s [-v] fontfile ...\n", program);
	exit(1);
}

showfont(file, verbose)
	char	*file;
	int	verbose;
{
	char	*buf;
	struct stat	st;
	TempFont	tf;
	FontInfoRec f;
	int	fd, i, strings;

	if (stat(file, &st) < 0) {
		fprintf(stderr, "can't stat %s\n", file);
		exit(1);
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "can't open %s\n", file);
		exit(1);
	}
	buf = malloc(st.st_size);
	read(fd, buf, st.st_size);
	close(fd);

	tf.pFI = (FontInfoPtr)buf;
	tf.pCI = (CharInfoPtr)(buf + BYTESOFFONTINFO(tf.pFI));
	tf.pGlyphs = ((unsigned char *)tf.pCI) + BYTESOFCHARINFO(tf.pFI);
	tf.pFP = (FontPropPtr)(tf.pGlyphs + BYTESOFGLYPHINFO(tf.pFI));
	strings = (int)tf.pFP + BYTESOFPROPINFO(tf.pFI);

	for (i=0; i<tf.pFI->nProps; i++) {
		tf.pFP[i].name += strings;
		if (tf.pFP[i].indirect)
			tf.pFP[i].value += strings;
	}
	printf("\n\n-------------  %s  ---------------\n\n", file);
	DumpFont(&tf, verbose);
	free(buf);
}
