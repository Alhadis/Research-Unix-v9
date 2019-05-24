SHELL=/bin/sh
PATH=:$HOME/bin:/bin:/usr/bin:/usr/ipc/bin:/usr/jtools/bin
MAIL=/usr/spool/mail/dak
PS1="`cat /etc/whoami`% "
DISPLAY=:0
HISTORY=$HOME/.hist
>$HISTORY
if [ ! "$REXEC" ]
then
	TERM=xterm
	stty crt erase 
fi
export TERM SHELL PATH PS1 DISPLAY HISTORY
umask 022
