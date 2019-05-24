#include <varargs.h>

#define	SIZE	1024
extern int	printcol;

char	*doprint();

print(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	char buf[SIZE], *out;

	va_start(args);
	fmt = va_arg(args, char *);
	va_end(args);
	out = doprint(buf, fmt, (char *)args);
	return write(1, buf, (int)(out-buf));
}

fprint(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	int f;
	char buf[SIZE], *out;

	va_start(args);
	f = va_arg(args, int);
	fmt = va_arg(args, char *);
	va_end(args);
	out = doprint(buf, fmt, (char *)args);
	return write(f, buf, (int)(out-buf));
}

sprint(va_alist)
va_dcl
{
	va_list args;
	char *buf;
	char *fmt;
	char *out;
	int scol;

	va_start(args);
	buf = va_arg(args, char *);
	fmt = va_arg(args, char *);
	va_end(args);
	scol = printcol;
	out = doprint(buf, fmt, (char *)args);
	printcol = scol;
	return out-buf;
}
