set path=(. /usr/athena /usr/athena/X.v9 /usr/new /usr/new/mh /usr/ucb /bin /usr/bin /usr/local /usr/clu/exe /usr/hosts)
unset time
unset	autologout
set history=50
umask 22
alias mail inc
alias	lj	jobs -l
alias	l	ls -l
alias	p	more
alias		clear
alias	delete	\'rm\'
alias	rm	rm -i
alias	.	source
alias	term	'setenv TERM `tset -Q - \!*`'
alias	sudo	bold -l root
alias	xx	tn xx
alias	opus	opus -l root
alias	milo	milo -l root
alias	xt	'xted =65+1+1&'
alias	xp	'xterm -fn 6x10 =40x72+1-1&'
alias	xp1	'xterm -fn 6x10 =40x72-1-1&'
alias	xm	'xwm -fz @5 fn=6x10&'
alias	rs	'set noglob;eval `resize`'
alias	xs	'xm;xt;xp'
alias	xrepl	'repl -ed xted'
alias	xhost	"prj 'setenv DISPLAY prj:1;xhost \!*'"
if ($?prompt) then
	set prompt="orpheus% "
endif
