#  C library -- umask
 
#  omask = umask(mode);
 
	umask = 60
.globl	_umask

_umask:
	pea	umask
	trap	#0
	rts
