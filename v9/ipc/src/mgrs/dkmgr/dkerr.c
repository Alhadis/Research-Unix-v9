#include <errno.h>

/* imported */
extern int errno;
extern char *errstr;

char *msgs[] = {
	"DK controller system error",
	"destination busy",
	"remote node not answering",
	"destination not answering",
	"unassigned destination",
	"DK system overload",
	"server already exists",
	"call rejected by destination",
};

int errs[] = {
	EIO,
	EBUSY,
	EIO,
	EIO,
	ENOENT,
	EBUSY,
	EEXIST,
	EACCES,
};

/*
 *  Convert a Unix error code to a dk error code
 */
int
utodkerr(uerr)
{
#ifdef LINT
	printf("%d\n", uerr);
#endif
	return 7;
}

/*
 *  Convert a dk error code to a Unix errstr and errno
 */
dktouerr(dkerr)
{
	dkerr &= 0x7f;
	if (dkerr<=0 || dkerr>sizeof(errs)/sizeof(int)+1) {
		errno = EGREG;
		errstr = "unknown error";
		return;
	}
	errno = errs[dkerr];
	errstr = msgs[dkerr];
}
