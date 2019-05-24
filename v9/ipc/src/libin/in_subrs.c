#include <ctype.h>
#include <sys/inet/in.h>

/* Get the next token in a string, returning a pointer the the byte
 * following the token.
 */
char *
in_getw(buf, w)
char *buf, *w;
{
	*w = 0;
	while(isspace(*buf)) buf++;
	if(*buf == '\0')
		return(0);
	while(!isspace(*buf) && *buf)
		*w++ = *buf++;
	*w = 0;
	return(buf);
}

/* get the network that a host is on */
in_addr
in_netof(x)
in_addr x;
{
	if(IN_CLASSC(x))
		return(x&IN_CLASSC_NET);
	else if(IN_CLASSB(x))
		return(x&IN_CLASSB_NET);
	else
		return(x&IN_CLASSA_NET);
}

