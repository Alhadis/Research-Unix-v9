/*
 * SUN-3 console driver
 */
#include "../machine/sunromvec.h"

/*
 * Print a character on console.
 */
cnputc(c)
	register int c;
{
	register int s;

	s = spl7();
	if (c == '\n')
		(*romp->v_putchar)('\r');
	(*romp->v_putchar)(c);
	(void) splx(s);
}

cngetc()
{
	register int c;

	while ((c = (*romp->v_mayget)()) == -1)
		;
	return (c);
}
