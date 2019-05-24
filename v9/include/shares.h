/*
**	Define format of data in ``shares'' file record.
*/

#ifndef	_TYPES_
#include	<sys/types.h>
#endif
#ifndef	_LNODE_
#include	<sys/lnode.h>
#endif

#define	SHAREFILE	"/etc/shares"
#define	MAXUID		10000
#define	OTHERUID	MAXUID		/* For spare lnode */
#define	OTHERSHARES	1
#define	OTHERGROUP	0
#define	IDLEUID		(MAXUID-1)	/* For ``idle'' lnode */
#define	IDLESHARES	0
#define	IDLEGROUP	0

typedef struct
{
	struct lnode	l;
	unsigned long	extime;
}
			Share;

typedef	Share *		ShareP;

extern int		ShareFd;

extern void		closeshares();
extern unsigned long	getshares();
extern unsigned long	getshput();
extern int		openshares();
extern int		putshares();
extern void		sharesfile();

#ifndef	NULLSTR
#define	NULLSTR		(char *)0
#endif
#ifndef	SYSERROR
#define	SYSERROR	(-1)
#endif
