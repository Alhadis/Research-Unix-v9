#  C library -- umask
 
#  omask = umask(mode);
 
	umask = 60
.globl	_umask

_umask:
	linkw	a6,#0
	pea	umask
	trap	#0
	unlk	a6
	rts
