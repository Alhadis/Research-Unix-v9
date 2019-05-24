# getpid -- get process ID

	getpid = 20
.globl	_getpid

_getpid:
	linkw	a6,#0
	pea	getpid
	trap	#0
	unlk	a6
	rts

.globl	_getppid

_getppid:
	linkw	a6,#0
	pea	getpid
	trap	#0
	movl	d1,d0
	unlk	a6
	rts
