char *malloc();

char *memalign(align, size)
unsigned align, size;
{
	int i = (int) malloc(size + align - 1);

	if (!i)
		return((char *)0);
	i += align - 1;
	i &= ~(align - 1);
	return ((char *)i);
}
