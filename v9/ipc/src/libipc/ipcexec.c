#include <ipc.h>
#include "defs.h"

/*
 *  Perform a remote execution of a command.
 */
ipcexec(name, param, cmd)
	char *name;
	char *param;
	char *cmd;
{
	return _ipcexec(name, "dk", "exec", param, cmd);
}

ipcmesgexec(name, param, cmd)
	char *name;
	char *param;
	char *cmd;
{
	return _ipcexec(name, "dk", "mesgexec", param, cmd);
}

_ipcexec(name, def, service, param, cmd)
	char *name;
	char *service;
	char *def;
	char *param;
	char *cmd;
{
	int fd, len;

	fd = ipcopen(ipcpath(name, def, service), param);
	if (fd<0)
		return -1;
	if (ipclogin(fd)<0) {
		close(fd);
		return -1;
	}
	len = strlen(cmd)+1;
	cmd[len-1] = '\n';
	if (write(fd, cmd, len)!=len) {
		errstr = "write error";
		close(fd);
		return -1;
	}
	cmd[len-1] = '\0';
	return fd;
}

ipcrexec(name, param, cmd)
	char *name;
	char *param;
	char *cmd;
{
	int fd;

	fd = ipcopen(ipcpath(name, "tcp", "tcp.514"), param);
	if (fd<0)
		return -1;
	if (ipcrogin(fd, cmd)<0) {
		close(fd);
		return -1;
	}
	return fd;
}
