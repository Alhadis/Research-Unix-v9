# C library -- getpagesize

	getpagesize = 64+46
.globl	_getpagesize

_getpagesize:
	linkw	a6,#0
	pea	getpagesize
	trap	#0
	unlk	a6
	rts
