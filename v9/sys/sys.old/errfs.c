/* error file system.  all it allows is iput.  Unmount in some file systems
 * (but not the default one) puts all inodes which are blocking the unmount
 * into this file system by changing their i_fstyp.  Then any use returns
 * errors.  The exact values of the error return have not been carefully
 * chosen.
 */
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
erget()
{	u.u_error = ENXIO;
}
erread()
{	u.u_error = ENXIO;
}
erwrite()
{
	u.u_error = ENXIO;
}
ertrunc()
{
	u.u_error = ENXIO;
}
erstat()
{
	u.u_error = ENXIO;
}
ernami()
{
	u.u_error = ENXIO;
	return(1);		/* see nami.c */
}
