/*
 * miscellany
 */

#include "mgr.h"
#include <setjmp.h>
#include <stdio.h>
#include <ctype.h>
#include <libc.h>

/*
 * trap errors from recomp and regexec
 */

jmp_buf rxerr;

regerror(s)
char *s;
{
	logevent("regex error: %s\n", s);
	longjmp(rxerr, 1);
}


/*
 * read a line from the remote end
 */
char *
rdline(f)
int f;
{
	static char buf[ARB];
	register char *p;
	register int n;

	for (p = buf; p-buf<ARB-1; p++) {
		n = read(f, p, 1);
		if (n <= 0)
			break;
		if (*p == '\r' || *p == '\n' || *p == '\0') {
			*++p = '\0';
			return (buf);
		}
	}
	return NULL;
}


/*
 *  compile a regular expression inserting a ^ at the beginning and
 *  a $ at the end.
 */
regexp *
nregcomp(re)
	char *re;
{
	char fullre[ARB];
	register char *cp=fullre;

	*cp++ = '^';
	while(*re)
		*cp++ = *re++;
	*cp++ = '$';
	*cp = '\0';
	return regcomp(fullre);
}
