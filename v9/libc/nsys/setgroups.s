# C library -- setgroups

.globl	_setgroups
.globl	cerror

	setgroups = 74

_setgroups:
	linkw	a6,#0
	pea	setgroups
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	unlk	a6
	rts
