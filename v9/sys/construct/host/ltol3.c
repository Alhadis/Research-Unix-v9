ltol3(cp, lp, n)
char	*cp;
long	*lp;
int	n;
{
	register i;
	register char *a, *b;

	a = cp;
	b = (char *)lp;
	for(i=0;i<n;i++) {
		b++;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
	}
}

l3tol(lp, cp, n)
long	*lp;
char	*cp;
int	n;
{
	register i;
	register char *a, *b;

	a = cp;
	b = (char *)lp;
	for(i=0;i<n;i++) {
		*b++ = 0;
		*b++ = *a++;
		*b++ = *a++;
		*b++ = *a++;
	}
}
