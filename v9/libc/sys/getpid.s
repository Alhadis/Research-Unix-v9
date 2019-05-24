# getpid -- get process ID

	getpid = 20
.globl	_getpid

_getpid:
	pea	getpid
	trap	#0
	rts

.globl	_getppid

_getppid:
	pea	getpid
	trap	#0
	movl	d1,d0
	rts
