# C library -- setgroups

.globl	_setgroups
.globl	cerror

	setgroups = 74

_setgroups:
	pea	setgroups
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	rts
