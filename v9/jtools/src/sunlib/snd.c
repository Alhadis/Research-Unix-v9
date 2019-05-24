#include "jerq.h"
#include <errno.h>

sendchar(c)
char c;
{
	sendnchars(1, &c);
}

sendnchars (n, p)
char *p;
int n;
{
	int i;
	int maxfd, rmask, wmask;

	maxfd = displayfd + 1;
	while(n){
		i = write(1, p, n);
		if(i > 0){
			n -= i;
			p += i;
			continue;
		}
#ifdef	BSD
		if(i < 0 && errno == EWOULDBLOCK){
#else	/* V9 */
		if(i < 0 && errno == EBUSY){
#endif
			do{
#ifdef X11
				while (XPending(dpy))
					handleinput();
#endif X11
				rmask = (1 << displayfd) | jerqrcvmask;
				wmask = 2;
#ifdef BSD
				select(maxfd, &rmask, &wmask, 0, 0);
#else
				select(maxfd, &rmask, &wmask, 0x6fffffff);
#endif
				if (rmask & jerqrcvmask)
					rcvfill();
				if (rmask & (1 << displayfd))
					handleinput();
			} while(!wmask);
		}
		else
			exit(1);
	}
}
