# C library -- getgroups

.globl	_getgroups
.globl	cerror

	getgroups = 75

_getgroups:
	linkw	a6,#0
	pea	getgroups
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	unlk	a6
	rts
