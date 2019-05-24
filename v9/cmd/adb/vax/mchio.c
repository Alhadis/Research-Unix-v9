/*
 * read or print machine-dependent data structures
 * this version for a vax running on a vax
 */

#include "defs.h"

/*
 * read an ASCII expression
 */

char lastc;

WORD
ascval()
{
	long l;
	register char *p;
	register int i;

	l = 0;
	p = (char *)&l;
	i = sizeof(l);
	while (quotchar()) {
		if (--i >= 0)
			*p++ = lastc;
	}
	return ((WORD)l);
}

/*
 * read a floating point number in VAX format
 * the result must fit in a WORD
 */

WORD
fpin(buf)
char *buf;
{
	union {
		WORD w;
		float f;
	} x;
	double atof();

	x.f = atof(buf);
	return (x.w);
}

/*
 * print a floating point number in VAX format
 */

#define	FPWID	32

fpout(flag, va)
char flag;
char *va;
{
	char buf[FPWID+1];
	char *gcvt();

	if ((*(unsigned short *)va & 0xff80) == 0x8000) {
		prints("illegal float   ");
		if (flag != 'f')
			prints("                ");	/* ugh */
		return;
	}
	if (flag == 'f')
		printf("%-16s", gcvt((double)*(float *)va, 9, buf));
	else
		printf("%-32s", gcvt(*(double *)va, 18, buf));
}
