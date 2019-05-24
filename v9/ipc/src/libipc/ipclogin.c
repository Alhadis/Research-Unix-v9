#include <sgtty.h>
#include <stdio.h>
#include <ipc.h>
#include "defs.h"
#include "libc.h"

/* this better be larger than either login names or passwords */
#define PWDSIZE 16

char *_ipcgetpass();

/*
 * log a user into a remote system in the v9 style
 *
 * First attempt at a valid userid came from ipcopen(),
 * who sent it as part of the setup request.  Remote
 * system responds to that with either "OK" or "NO".
 * If the latter, we must send a string containing
 * "userid,password<newline>" until either we receive
 * the OK response or our user here gives up.
 */
ipclogin(fd)
{
#	define BFS 2*PWDSIZE + 3	/* <log>,<pswd>\n\0 */
	int	cc;
	struct sgttyb echo, noecho;
	char	buf[BFS];
	char	*rbuf;
	FILE	*tty;

	cc = read(fd, buf, 2) ;
	if (cc <= 0) {
		errstr = "ipclogin can't read remote system";
		return(-1);
	}
	if (buf[0] == 'O' && buf[1] == 'K')
		return fd ;
	tty = fopen("/dev/tty", "r+");
	if (tty == NULL) {
		errstr = "ipclogin can't open /dev/tty";
		return -1;
	}
	if (ioctl(fileno(tty), TIOCGETP, &echo) < 0) {
		errstr = "ipclogin can't log in";
		return(-1);
	}
	noecho = echo;
	noecho.sg_flags &= ~ECHO;
	write (fileno(tty), "please ", 7) ;
	while (1) {
		rbuf = _ipcgetpass (tty, "login: ");
		if (rbuf == 0) {
			cc = -1;
			break;
		}
		strcpy (buf, rbuf);
		strcat (buf, ",");

		ioctl(fileno(tty), TIOCSETP, &noecho) ;
		rbuf = _ipcgetpass (tty, "Password:");
		ioctl(fileno(tty), TIOCSETP, &echo) ;
		write(fileno(tty), "\n", 1);
		if (rbuf == 0) {
			cc = -2;
			break;
		}
		strcat (buf, rbuf);
		strcat (buf, "\n");

		write(fd, buf, strlen (buf)) ;
		cc = read(fd, buf, 2) ;
		if (cc <= 0)
			break ;
		if (buf[0] == 'O' && buf[1] == 'K')
			break ;
	}
	fclose(tty);
	if (cc <= 0) {
		close(fd);
		errstr = "ipclogin can't log in";
		return -1;
	}
	return fd;
}

/*
 * log a user into a remote system in the Berkeley style
 *
 * Send a string to the other side of the form
 *	"<NULL>local_user_name<NULL>remote_user_name<NULL>terminal_type<NULL>"
 * The response is either
 * 	"<NULL>"		to indicate success or
 *	"<non_0_byte>string"	where `string' is the error message
 */
ipcrogin(fd, opt)
	int fd;
	char *opt;
{
	char buf[32];
	char *me=getlogin();
	int len;

	len = me!=NULL ? strlen(me) : 0;
	write(fd, "", 1);
	write(fd, me, len+1);
	write(fd, me, len+1);
	write(fd, opt, strlen(opt)+1);
	if(read(fd, buf, 1) != 1)
		return -1;
	if(buf[0] != 0){
		while(read(fd, buf, 1) == 1){
			write(2, buf, 1);
			if(buf[0] == '\n')
				break;
		}
		return -1;
	}
	return 0;
}

char *
_ipcgetpass(tty, prompt)
FILE *tty;
char * prompt;
{
	char c;
	char *cp;
	static char buf[PWDSIZE + 1];

	write (fileno(tty), prompt, strlen (prompt));

	cp = buf;
	while ((c = getc(tty)) != '\n' && c != EOF && c != '\04') {
		if (cp < &buf[PWDSIZE])
			*cp++ = c;
	}
	*cp = '\0';

	if (buf[0] == '\0' && (c == EOF || c== '\04'))
		return ((char *)0);
	if (buf[0] == '~' && buf[1] == '.')
		return ((char *)0);

	return (buf);
}
