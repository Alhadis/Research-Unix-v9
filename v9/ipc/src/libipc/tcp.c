#include "defs.h"
#include <ctype.h>

/*
 *  convert a port number into a file system name
 */
char *
tcptofs(port)
	int port;
{
	static char name[PATHLEN];

	sprintf(name, "/cs/tcp.%d", port);
	return name;
}

/*
 *  convert an internal name into a port number
 */
fstotcp(name)
	char *name;
{
	int port;

	if (strncmp(name, CSROOT, sizeof(CSROOT)-1)==0)
		name += sizeof(CSROOT)-1;
	if (strncmp(name, "tcp.", sizeof("tcp.")-1)==0)
		name += sizeof("tcp.")-1;
	for (port=0; *name; name++) {
		if (!isdigit(*name))
			return -1;
		port = port*10 + (*name - '0');
	}
	return port;
}

/*
 *  ipccreate a tcp usable port.  a bit of a crock.
 */
tcpcreat(name, param)
	char *name;
	char *param;
{
	int p;
	int offset;
	int fd= -1;

	if (getuid()==0)
		offset = 512;
	else
		offset = 1024;
	for (p=0; p<511 && fd<0; p++) {
		strcpy(name, tcptofs(p+offset));
		fd = ipccreat(name, param);
		if (fd<0)
			continue;
	}
	return fd;
}
