#include "stream.h"

/* define some UNIX calls */
extern int open (char *, int);
extern int close (int);
extern long lseek (int, long, int);
extern int read (int, char *, unsigned);
extern int write (int, char *, unsigned);
extern int creat (char *, int);

/*
 *	Open a file with the given mode.
 *	Return:		NULL if failure
 *			this if success
 */
filebuf* filebuf::open (char *name, open_mode om)
{
	switch (om) {
	case input:	
		fd = ::open(name, 0);
		break;
	case output:	
		fd = creat(name, 0664);
		break;
	case append:	
		fd = ::open(name, 1);
		if (fd < 0) fd = creat(name, 0664);
		if (fd >= 0) (void)lseek(fd, 0, 2);
		break;
	}

	if (fd < 0) return NULL;

	opened = 1;
	return this;
}

/*
 *	Empty an output buffer.
 *	Returns:	EOF on error
 *			0 on success
 */
int filebuf::overflow(int c)
{
	if (!opened || allocate()==EOF) return EOF;

	if (fp != NULL) {		// stdio compatibility
		fflush(fp);
		return 0;
	}
	else if (base == eptr) {	// unbuffered IO
		if (c != EOF) {
			*pptr = c;
			if (write(fd, pptr, 1) != 1) return EOF;
		}
	}
	else {				// buffered IO
		if (pptr > base)
			if (write(fd, base, pptr-base) != pptr-base) return EOF;
		pptr = gptr = base;
		if (c != EOF) *pptr++ = c;
	}
	return c & 0377;
}

/*
 *	Fill an input buffer.
 *	Returns:	EOF on error or end of input
 *			next character on success
 */
int filebuf::underflow()
{
	int count;
	extern int strlen(char *);

	if (!opened || allocate()==EOF) return EOF;

	if (fp!=NULL) {			// stdio compatibility
		if (fgets(base+1, eptr-base-1, fp) == NULL) return EOF;
		count = strlen(base+1);
	}
	else {				// normal stream io
		if ((count=read(fd, base+1, eptr-base-1)) < 1) return EOF;
	}
	gptr = base+1;			// leave room for putback
	pptr = gptr+count;
	return *gptr & 0377;
}
