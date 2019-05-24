# C library -- getgroups

.globl	_getgroups
.globl	cerror

	getgroups = 75

_getgroups:
	pea	getgroups
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	rts
