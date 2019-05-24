/*
 * Memory operations as defined in the 8th edition manual
 * D .A. Kapilow		12/17/86
 */

char *memccpy(s1, s2, c, n)
register char *s1, *s2;
register int c, n;
{
	register char cc = c;

	for (; n--; s1++, s2++)
		if (cc == (*s1 = *s2))
			return ++s1;
	return (char *)0;
}

char *memchr(s, c, n)
register char *s;
register c, n;
{
	register char cc = c;

	for (; n--; s++)
		if (*s == cc)
			return s;
	return (char *)0;		
}

int memcmp(s1, s2, n)
register char *s1, *s2;
register int n;
{
	for (; n--; s1++, s2++)
		if (*s1 != *s2)
			return (*s1 - *s2);
	return 0;
}

char *memcpy(s1, s2, n)
register char *s1, *s2;
register int n;
{
	char *r = s1;

	while(n--)
		*s1++ = *s2++;
	return r;
}

char *memset(s, c, n)
register char *s;
register int c, n;
{
	char *r = s;

	while (n--)
		*s++ = c;
	return r;
}
