# C library -- getpagesize

	getpagesize = 64+46
.globl	_getpagesize

_getpagesize:
	pea	getpagesize
	trap	#0
	rts
