/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %D are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much
 * suspended.
 * Printf should not be used for chit-chat.
 */
/* VARARGS1 */
_printf(fmt, x1)
register char *fmt;
unsigned x1;
{
	register c;
	register unsigned *adx;
	char *s;

	adx = &x1;
loop:
	while((c = *fmt++) != '%') {
		if(c == '\0')
			return;
		_putchar(c);
	}
	c = *fmt++;
	if(c == 'd' || c == 'u' || c == 'o' || c == 'x')
		_printn((long)*adx, c=='o'? 8: (c=='x'? 16:10));
	else if(c == 's') {
		s = (char *)*adx;
		while(c = *s++)
			_putchar(c);
		adx++;
	} else if (c=='D' || c=='O' || c =='X') {
		_printn(*(long *)adx, c=='O'? 8: (c=='X'? 16:10));
		adx += (sizeof(long) / sizeof(int)) - 1;
	}
	adx++;
	goto loop;
}

/*
 * Print an unsigned integer in base b.
 */
_printn(n, b)
long n;
{
	register long a;

	if (n<0) {	/* shouldn't happen */
		_putchar('-');
		n = -n;
	}
	if(a = n/b)
		_printn(a, b);
	_putchar("0123456789ABCDEF"[(int)(n%b)]);
}

_putchar(c)
char c;
{
	write(1,&c,1);
}
