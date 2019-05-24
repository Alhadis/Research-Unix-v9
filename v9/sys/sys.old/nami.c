#include "../h/param.h"
#include "../h/systm.h"
#include "../h/inode.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"

/*
 * Convert a pathname into a pointer to
 * a locked inode.
 *
 * func = function called to get next char of name
 *	&uchar if name is in user space
 *	&schar if name is in system space
 *	length guaranteed <= BUFSIZE if func != uchar
 * flagp
	0 for ordinary searches
	else ->flag structure with more parameters
 * follow = 1 if links are to be followed at the end of the name
 */

struct inode *
namei(func, flagp, follow)
int		(*func)();
struct argnamei	*flagp;
int		follow;
{
	register int	i;
	register char	*cp;
	struct nx	p;

	p.nbp = geteblk();
	if(func == uchar) {
		if((i = fustrlen(u.u_dirp)) < 0) {
			u.u_error = EFAULT;
			brelse(p.nbp);
			return NULL;
		}
		if(i > BUFSIZE) {
			u.u_error = ENOENT;
			brelse(p.nbp);
			return NULL;
		}
		bcopy(u.u_dirp, p.nbp->b_un.b_addr, i);
#ifdef	CHAOS
		u.u_dirp += i;
#endif
	} else {
		cp = p.nbp->b_un.b_addr;
		do; while(*cp++ = (*func)());
	}
	if(flagp != NULL) {
		cp = p.nbp->b_un.b_addr;
		while(i = *cp++) {
			if(i & 0200) {
				u.u_error = ENOENT;
				brelse(p.nbp);
				return NULL;
			}
		}
	}
	cp = p.nbp->b_un.b_addr;
	if(*cp == '/') {
		while(*cp == '/')
			cp++;
		if((p.dp = u.u_rdir) == NULL)
			p.dp = rootdir;
	} else
		p.dp = u.u_cdir;
	p.nlink = 0;
	p.cp = cp;
	plock(p.dp);
	p.dp->i_count++;

	for (;;) {
		switch((*fstypsw[p.dp->i_fstyp].t_nami)(&p, &flagp, follow)) {
		case 1:
			iput(p.dp);
			brelse(p.nbp);
			return NULL;
		case 2:
			if(*p.cp)
				break;
		case 0:
			brelse(p.nbp);
			return p.dp;
		default:
			panic("namei ret");
		}
	}
}

/*
 * for filesystems without namei code
 */

int
nullnami()
{
	u.u_error = ENODEV;
	return (1);
}

/*
 * Return the next character from the
 * kernel string pointed at by dirp.
 */
schar()
{

	return(*u.u_dirp++ & 0377);
}

/*
 * Return the next character from the
 * user string pointed at by dirp.
 */
uchar()
{
	register c;

	c = fubyte(u.u_dirp++);
	if(c == -1) {
		u.u_error = EFAULT;
		c = 0;
	}
	return(c);
}
