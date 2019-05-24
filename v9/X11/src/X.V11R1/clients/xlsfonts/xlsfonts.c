#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include "dsimple.h"

#define N_START 1000  /* Maximum # of fonts to start with */

int	long_list;
int	force_columns;
int	force_1column;
int	nnames = N_START;
int	font_cnt;
int	min_max;
typedef struct {
	char		*name;
	XFontStruct	*info;
} FontList;
FontList	*font_list;


usage()
{
	fprintf(stderr,"%s: usage: %s [host:display] [pattern]\n", program_name,
		program_name);
	exit(1);
}

main(argc, argv)
int argc;
char **argv;    
{
	int	argcnt, i;

	INIT_NAME;

	/* Handle command line arguments, open display */
	Setup_Display_And_Screen(&argc, argv);
	for (argv++, argc--; argc; argv++, argc--) {
		if (argv[0][0] == '-') {
			for (i=1; argv[0][i]; i++)
				switch(argv[0][i]) {
				case 'l':
					long_list++;
					break;
				case 'm':
					min_max++;
					break;
				case 'C':
					force_columns++;
					force_1column = 0;
					break;
				case '1':
					force_1column++;
					force_columns = 0;
					break;
				default:
					usage();
					break;
				}
			if (i == 1)
				usage();
		} else {
			argcnt++;
			get_list(argv[0]);
		}
	}
	if (argcnt == 0)
		get_list("*");
	show_fonts();
	exit(0);
}

get_list(pattern)
	char	*pattern;
{
	int	available = nnames+1,
		i;
	char	**fonts;
	XFontStruct	*info;

	/* Get list of fonts matching pattern */
	for (;;) {
		if (long_list)
			fonts = XListFontsWithInfo(dpy,
				pattern, nnames, &available, &info);
		else
			fonts = XListFonts(dpy, pattern, nnames, &available);
		if (fonts == NULL || available < nnames)
			break;
		if (long_list)
			XFreeFontInfo(fonts, info, available);
		else
			XFreeFontNames(fonts);
		nnames = available * 2;
	}

	if (fonts == NULL) {
		fprintf(stderr, "%s: pattern \"%s\" unmatched\n",
			program_name, pattern);
		return;
	}

	font_list = (FontList *)Realloc(font_list,
		(font_cnt + available) * sizeof(FontList));
	for (i=0; i<available; i++) {
		font_list[font_cnt].name = fonts[i];
		if (long_list)
			font_list[font_cnt].info = info + i;
		else
			font_list[font_cnt].info = NULL;
		font_cnt++;
	}
}

compare(f1, f2)
	FontList	*f1, *f2;
{
	char	*p1 = f1->name,
		*p2 = f2->name;

	while (*p1 && *p2 && *p1 == *p2)
		p1++, p2++;
	return(*p1 - *p2);
}

show_fonts()
{
	int	i;

	if (font_cnt == 0)
		return;

	/* first sort the output */
	qsort(font_list, font_cnt, sizeof(FontList), compare);

	if (long_list) {
		XFontStruct	*pfi;
		char		*string;

		printf("DIR  ");
		printf("MIN  ");
		printf("MAX ");
		printf("EXIST ");
		printf("DFLT ");
		printf("PROP ");
		printf("ASC ");
		printf("DESC ");
		printf("NAME");
		printf("\n");
		for (i=0; i<font_cnt; i++) {
			pfi = font_list[i].info;
			switch(pfi->direction) {
			case FontLeftToRight: string = "-->"; break;
			case FontRightToLeft: string = "<--"; break;
			default:	      string = "???"; break;
			}
			printf("%-4s", string);
			if (pfi->min_byte1 == 0
			 && pfi->max_byte1 == 0) {
				printf(" %3d ", pfi->min_char_or_byte2);
				printf(" %3d ", pfi->max_char_or_byte2);
			} else {
				printf("*%3d ", pfi->min_byte1);
				printf("*%3d ", pfi->max_byte1);
			}
			printf("%5s ", pfi->all_chars_exist ? "all" : "some");
			printf("%4d ", pfi->default_char);
			printf("%4d ", pfi->n_properties);
			printf("%3d ", pfi->ascent);
			printf("%4d ", pfi->descent);
			printf("%s\n", font_list[i].name);
			if (min_max) {
				char	min[ BUFSIZ ],
					max[ BUFSIZ ];
				char	*pmax = max,
					*pmin = min;
				int	w;

				strcpy(pmin, "     min(l,r,w,a,d) = (");
				strcpy(pmax, "     max(l,r,w,a,d) = (");
				pmin += strlen(pmin);
				pmax += strlen(pmax);

				copy_number(&pmin, &pmax,
					pfi->min_bounds.lbearing,
					pfi->max_bounds.lbearing);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.rbearing,
					pfi->max_bounds.rbearing);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.width,
					pfi->max_bounds.width);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.ascent,
					pfi->max_bounds.ascent);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.descent,
					pfi->max_bounds.descent);
				*pmin++ = *pmax++ = ')';
				*pmin = *pmax = '\0';
				printf("%s\n", min);
				printf("%s\n", max);
			}
		}
		return;
	}

	if ((!force_1column && isatty(1)) || force_columns) {
		int	width,
			max_width = 0,
			columns,
			lines_per_column,
			j,
			index;

		for (i=0; i<font_cnt; i++) {
			width = strlen(font_list[i].name);
			if (width > max_width)
				max_width = width;
		}
		if (max_width == 0)
			Fatal_Error("Max width of font names is 0!");
		max_width += 3;
		columns = (79 + max_width - 1) / max_width;
		if (font_cnt < columns)
			columns = font_cnt;
		lines_per_column = (font_cnt + columns - 1) / columns;

		for (i=0; i<lines_per_column; i++) {
			for (j=0; j<columns; j++) {
				index = j * lines_per_column + i;
				if (index >= font_cnt)
					break;
				if (j+1 == columns)
					printf("%s", font_list[ index ]);
				else
					printf("%-*s",
						max_width, font_list[ index ]);
			}
			printf("\n");
		}
		return;
	}

	for (i=0; i<font_cnt; i++)
		printf("%s\n", font_list[i].name);
}

max(i, j)
	int	i, j;
{
	if (i > j)
		return (i);
	return(j);
}

copy_number(pp1, pp2, n1, n2)
	char	**pp1, **pp2;
	int	n1, n2;
{
	char	*p1 = *pp1;
	char	*p2 = *pp2;
	int	w;

	sprintf(p1, "%d", n1);
	sprintf(p2, "%d", n2);
	w = max(strlen(p1), strlen(p2));
	sprintf(p1, "%*d", w, n1);
	sprintf(p2, "%*d", w, n2);
	p1 += strlen(p1);
	p2 += strlen(p2);
	*pp1 = p1;
	*pp2 = p2;
}
